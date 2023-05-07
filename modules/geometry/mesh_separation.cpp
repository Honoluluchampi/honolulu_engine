// hnll
#include <geometry/mesh_separation.hpp>
#include <geometry/primitives.hpp>
#include <geometry/he_mesh.hpp>
#include <geometry/bounding_volume.hpp>
#include <geometry/intersection.hpp>
#include <graphics/graphics_models/static_mesh.hpp>
#include <graphics/graphics_models/static_meshlet.hpp>
#include <graphics/utils.hpp>
#include <utils/utils.hpp>

// std
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <chrono>

namespace hnll::geometry {

#define FRAME_SAMPLING_STRIDE 37

// colors are same as mesh shader
#define COLOR_COUNT 10
vec3 meshlet_colors[COLOR_COUNT] = {
  // yumekawa
  vec3(0.75, 1,    0.5),
  vec3(0.75, 0.5,  1),
  vec3(0.5,  1,    0.75),
  vec3(0.5,  0.75, 1),
  vec3(1,    0.75, 0.5),
  vec3(1,    0.5,  0.75),
  vec3(0.5,  1,    1),
  vec3(0.5,  0.5,  1),
  vec3(0.5,  1,    0.5),
  vec3(1,    0.5,  0.5)
};

void colorize_meshlets(std::vector<s_ptr<mesh_model>>& meshlets)
{
  int i = 0;
  for (auto& m : meshlets) {
    m->he_mesh.colorize_whole_mesh(meshlet_colors[i++].cast<double>());
    i %= COLOR_COUNT;
  }
}

face_id separation_helper::get_random_remaining_face_id()
{
  for (const auto& id : remaining_face_ids_) {
    return id;
  }
  return static_cast<face_id>(-1);
}

s_ptr<separation_helper> separation_helper::create(
  const s_ptr<mesh_model> &model,
  const std::string& _model_name,
  mesh_separation::criterion _crtr)
{
  auto helper_sp = std::make_shared<separation_helper>(model);
  helper_sp->set_model_name(_model_name);
  helper_sp->set_criterion(_crtr);
  return helper_sp;
}

separation_helper::separation_helper(const s_ptr<mesh_model> &model)
  : model_(model), vertex_map_(model->get_vertex_map()), face_map_(model->get_face_map())
{
  for (const auto& fc_kv : face_map_) {
    remaining_face_id_set_.insert(fc_kv.first);
  }

  unsigned int absent_pair_count = 0;
  for (const auto& he_kv : model_->get_half_edge_map()) {
    if (he_kv.second->get_pair() == nullptr) {
      absent_pair_count++;
    }
  }
  std::cout << "odd h-edge count : " << absent_pair_count << std::endl;
  std::cout << "vertex count     : " << vertex_map_.size() << std::endl;
  std::cout << "face count       : " << face_map_.size() << std::endl;
  std::cout << "h-edge count     : " << model_->get_half_edge_map().size() << std::endl;
}


double separation_helper::compute_loss(const aabb& curr_aabb, face_id f_id)
{
  const auto& new_f = get_face(f_id);
  auto f_aabb = create_bv_from_single_face<bv_type::AABB>(new_f);
  double max_x = std::max(curr_aabb.get_max_x(), f_aabb->get_max_x());
  double min_x = std::min(curr_aabb.get_min_x(), f_aabb->get_min_x());
  double max_y = std::max(curr_aabb.get_max_y(), f_aabb->get_max_y());
  double min_y = std::min(curr_aabb.get_min_y(), f_aabb->get_min_y());
  double max_z = std::max(curr_aabb.get_max_z(), f_aabb->get_max_z());
  double min_z = std::min(curr_aabb.get_min_z(), f_aabb->get_min_z());
  auto x_diff = max_x - min_x;
  auto y_diff = max_y - min_y;
  auto z_diff = max_z - min_z;

  auto ret = std::max(x_diff, y_diff);
  ret = std::max(ret, z_diff);

  return ret;
}

double dist2(const vec3d& a, const vec3d& b)
{
  auto ba = b - a;
  return ba.x() * ba.x() + ba.y() * ba.y() + ba.z() * ba.z();
}

double separation_helper::compute_loss(const b_sphere& curr_s, face_id f_id)
{
  const auto& new_f = get_face(f_id);
  const auto& he = he_map_.at(new_f.he_id);
  const auto& next = he_map_.at(he.next);
  const auto& prev = he_map_.at(he.prev);

  auto radius2 = std::pow(curr_s.get_sphere_radius(), 2);
  radius2 = std::max(radius2, dist2(curr_s.get_local_center_point(), v_map_.at(he.v_id).position));
  radius2 = std::max(radius2, dist2(curr_s.get_local_center_point(), v_map_.at(next.v_id).position));
  radius2 = std::max(radius2, dist2(curr_s.get_local_center_point(), v_map_.at(prev.v_id).position));
  return radius2;
}

template <bv_type type>
face_id choose_best_face_id(const face_id_set& adjoining_face_ids, const bounding_volume<type>& bv)
{
  double loss = 1e9 + 7;
  face_id ret = -1;
  for (auto fc_id : adjoining_face_ids) {
    auto new_loss = compute_loss(bv, fc_id);
    if (new_loss < loss) {
      loss = new_loss;
      ret = fc_id;
    }
  }
  return ret;
}

void add_face_to_meshlet(const s_ptr<face>& fc, s_ptr<mesh_model>& ml)
{
  auto he = fc->half_edge_;
  auto v0 = he->get_vertex();
  he = he->get_next();
  auto v1 = he->get_vertex();
  he = he->get_next();
  auto v2 = he->get_vertex();
  ml->add_face(v0, v1, v2, fc->id_, false);
}

void update_aabb(bounding_volume& current_aabb, const s_ptr<face>& new_face)
{
  auto face_aabb = create_aabb_from_single_face(new_face);
  vec3d max_vec3, min_vec3;
  max_vec3.x() = std::max(current_aabb.get_max_x(), face_aabb->get_max_x());
  min_vec3.x() = std::min(current_aabb.get_min_x(), face_aabb->get_min_x());
  max_vec3.y() = std::max(current_aabb.get_max_y(), face_aabb->get_max_y());
  min_vec3.y() = std::min(current_aabb.get_min_y(), face_aabb->get_min_y());
  max_vec3.z() = std::max(current_aabb.get_max_z(), face_aabb->get_max_z());
  min_vec3.z() = std::min(current_aabb.get_min_z(), face_aabb->get_min_z());
  auto center_vec3 = (max_vec3 + min_vec3) / 2.f;
  auto radius_vec3 = max_vec3 - center_vec3;
  current_aabb.set_center_point(center_vec3);
  current_aabb.set_aabb_radius(radius_vec3);
}

void update_sphere(bounding_volume& current_sphere, const s_ptr<face>& new_face)
{
  // calc farthest point
  int far_id = -1;
  auto he = new_face->half_edge_;
  auto far_dist2 = std::pow(current_sphere.get_sphere_radius(), 2);
  if (auto dist = calc_dist(current_sphere.get_local_center_point(), he->get_vertex()->position_); dist > far_dist2 ){
    far_dist2 = dist;
    far_id = 0;
  }
  if (auto dist = calc_dist(current_sphere.get_local_center_point(), he->get_next()->get_vertex()->position_); dist > far_dist2 ){
    far_dist2 = dist;
    far_id = 1;
  }
  if (auto dist = calc_dist(current_sphere.get_local_center_point(), he->get_next()->get_next()->get_vertex()->position_); dist > far_dist2 ){
    far_dist2 = dist;
    far_id = 2;
  }

  if (far_id == -1) return;

  vec3d new_position;
  for (int i = 0; i < far_id; i++) {
    he = he->get_next();
  }
  new_position = he->get_vertex()->position_;

  // compute new center and radius
  auto center = current_sphere.get_local_center_point();
  auto radius = current_sphere.get_sphere_radius();
  auto new_dir = (new_position - center).normalized();
  auto opposite = center - radius * new_dir;
  auto new_center = 0.5f * (new_position + opposite);
  current_sphere.set_center_point(new_center);

  auto new_radius = std::sqrt(calc_dist(new_center, new_position));
  current_sphere.set_sphere_radius(new_radius);
}

void separation_helper::update_adjoining_face_map(face_map& adjoining_face_map, const s_ptr<face>& fc)
{
  auto first_he = fc->half_edge_;
  auto current_he = first_he;
  do {
    auto he_pair = current_he->get_pair();
    if (he_pair != nullptr) {
      auto current_face = current_he->get_pair()->get_face();
      // if current_face is new to the map
      if (remaining_face_id_set_.find(current_face->id_) != remaining_face_id_set_.end()) {
        adjoining_face_map[current_face->id_] = current_face;
      }
    }
    current_he = current_he->get_next();
  } while (current_he != first_he);
  adjoining_face_map.erase(fc->id_);
}

s_ptr<face> choose_random_face_from_map(const face_map& fc_map)
{
  for (const auto& fc_kv : fc_map) {
    return fc_kv.second;
  }
  return nullptr;
}

bool check_adjoining_face(const s_ptr<half_edge>& he, const separation_helper& helper)
{
  // no pair
  if (!he->get_pair())
    return false;
  const auto& pair = he->get_pair();
  // no face
  if (!pair->get_face())
    return false;
  return helper.face_is_remaining(pair->get_face()->id_);
}

s_ptr<face> choose_next_face_from_adjoining(const face_map& fc_map, const separation_helper& helper)
{
  int max_adjoining = 0;
  s_ptr<face> ret = nullptr;
  for (const auto& fc_kv : fc_map) {
    int adjoining = 0;
    auto& he = fc_kv.second->half_edge_;
    if (!check_adjoining_face(he, helper))
      adjoining++;
    he = he->get_next();
    if (!check_adjoining_face(he, helper))
      adjoining++;
    he = he->get_next();
    if (!check_adjoining_face(he, helper))
      adjoining++;

    if (max_adjoining < adjoining)
      ret = fc_kv.second;
  }
  return ret;
}

template<bv_type type>
std::vector<s_ptr<mesh_model<type>>> separate_greedy(const s_ptr<separation_helper>& helper)
{
  std::vector<s_ptr<mesh_model<type>>> meshlets;
  auto crtr = helper->get_criterion();
  auto curr_f_id = helper->get_random_remaining_face_id();
  const auto& curr_f = helper->get_face(curr_f_id);

  while (!helper->all_face_is_registered()) {
    // compute each meshlet
    // init objects
    auto ml = mesh_model<type>::create();
    auto bv = helper->create_bv_from_single_face<type>(curr_f);

    face_id_set adjoining_face_ids {curr_f_id};

    while (ml->get_vertex_count() < graphics::meshlet_constants::MAX_VERTEX_COUNT
           && ml->get_face_count() < graphics::meshlet_constants::MAX_PRIMITIVE_COUNT
           && adjoining_face_ids.size() != 0) {

      // algorithm dependent part
      curr_f_id = choose_best_face_id(adjoining_face_ids, *bv);

      // update each object
      add_face_to_meshlet(current_face, ml);
      if (crtr == mesh_separation::criterion::MINIMIZE_AABB)
        update_aabb(*bv, current_face);
      if (crtr == mesh_separation::criterion::MINIMIZE_BOUNDING_SPHERE)
        update_sphere(*bv, current_face);
      helper->update_adjoining_face_map(adjoining_face_map, current_face);
      helper->remove_face(current_face->id_);
    }
    ml->set_bounding_volume(std::move(bv));
    meshlets.emplace_back(ml);
    current_face = choose_next_face_from_adjoining(adjoining_face_map, *helper);
    if (current_face == nullptr)
      current_face = helper->get_random_remaining_face();
  }

  return meshlets;
}

//template<typename T>
//T my_max(const std::vector<T>& values)
//{
//  auto ret = std::numeric_limits<T>::min();
//  for (const auto& value : values)
//    if (value > ret)
//      ret = value;
//  return ret;
//}
//
//template<typename T>
//T my_sum(const std::vector<T>& values)
//{
//  T ret = 0;
//  for (const auto& value : values)
//    ret += value;
//  return ret;
//}
//
//face_id choose_the_best_face_for_animation(
//  const face_map& adjoining_face_map,
//  const std::vector<u_ptr<bounding_volume>>& bvs,
//  const std::vector<s_ptr<separation_helper>>& helpers,
//  int sampling_stride)
//{
//  // update these
//  double loss = 2e9;
//  face_id ret_id = 0;
//
//  size_t frame_count = bvs.size();
//
//  for (const auto& fc_kv : adjoining_face_map) {
//    auto id = fc_kv.first;
//    std::vector<double> new_loss_list;
//    for (int i = 0; i < frame_count; i += sampling_stride) {
//      new_loss_list.emplace_back(compute_loss_function_for_sphere(*bvs[i], helpers[i]->get_face(id)));
//    }
//    // max
//    auto new_loss = my_max<double>(new_loss_list);
//    // sum
////    auto new_loss = my_sum<double>(new_loss_list);
//
//    if (loss > new_loss) {
//      loss = new_loss;
//      ret_id = id;
//    }
//  }
//
//  return ret_id;
//}
//
//std::vector<s_ptr<mesh_model>> separate_animation_greedy(const std::vector<s_ptr<separation_helper>>& helpers)
//{
//  std::vector<s_ptr<mesh_model>> meshlets;
//
//  // extract representative
//  auto rep = helpers[0];
//  uint32_t frame_count = helpers.size();
//  auto crtr = rep->get_criterion();
//  s_ptr<face> current_face = rep->get_random_remaining_face();
//  auto current_face_id = current_face->id_;
//
//  while (!rep->all_face_is_registered()) {
//    // compute each meshlet
//    // init objects
//    s_ptr<mesh_model> ml = mesh_model::create();
//
//    // create bounding spheres for each frame
//    std::vector<u_ptr<bounding_volume>> bvs;
//    bvs.resize(frame_count);
//    for (int i = 0; i < frame_count; i++) {
//      bvs[i] = create_b_sphere_from_single_face(helpers[i]->get_face(current_face_id));
//    }
//
//    face_map adjoining_face_map {{current_face->id_ ,current_face}};
//
//    // meshlet api limitation
//    while (ml->get_vertex_count() < graphics::meshlet_constants::MAX_VERTEX_COUNT
//           && ml->get_face_count() < graphics::meshlet_constants::MAX_PRIMITIVE_COUNT
//           && adjoining_face_map.size() != 0) {
//
//      current_face_id = choose_the_best_face_for_animation(adjoining_face_map, bvs, helpers, FRAME_SAMPLING_STRIDE);
//      current_face = rep->get_face(current_face_id);
//
//      // update each object
//      add_face_to_meshlet(current_face, ml);
//      for (int i = 0; i < frame_count; i++) {
//        update_sphere(*bvs[i], helpers[i]->get_face(current_face_id));
//      }
//      rep->update_adjoining_face_map(adjoining_face_map, current_face);
//      rep->remove_face(current_face->id_);
//    }
//    ml->set_bounding_volumes(std::move(bvs));
//    meshlets.emplace_back(ml);
//    current_face = choose_random_face_from_map(adjoining_face_map);
//    if (current_face == nullptr)
//      current_face = rep->get_random_remaining_face();
//  }
//
//  return meshlets;
//}

graphics::meshlet translate_to_meshlet(const s_ptr<mesh_model>& old_mesh)
{
  graphics::meshlet ret{};

  std::unordered_map<uint32_t, uint32_t> id_map;
  uint32_t current_id = 0;

  // fill vertex info
  for (const auto& v_kv : old_mesh->get_vertex_map()) {
    auto& v = v_kv.second;
    // register to id_map
    id_map[v->id_] = current_id;
    // add to meshlet
    ret.vertex_indices[current_id++] = v->id_;
  }
  ret.vertex_count = current_id;

  current_id = 0;
  // fill primitive index info
  for (const auto& f_kv : old_mesh->get_face_map()) {
    auto& f = f_kv.second;
    auto& he = f->half_edge_;
    for (int i = 0; i < 3; i++) {
      ret.primitive_indices[current_id++] = id_map[he->get_vertex()->id_];
      he = he->get_next();
    }
  }
  ret.index_count = current_id;

  // fill bounding volume info
  ret.center = old_mesh->get_bounding_volume().get_local_center_point().cast<float>();
  ret.radius = old_mesh->get_bounding_volume().get_sphere_radius();

  return ret;
}

//graphics::animated_meshlet_pack translate_to_animated_meshlet_pack(const std::vector<s_ptr<mesh_model>>& old_meshes)
//{
//  graphics::animated_meshlet_pack meshlet_pack;
//
//  size_t frame_count = old_meshes[0]->get_bounding_volumes().size();
//  meshlet_pack.spheres.resize(frame_count);
//
//  for (const auto& old_mesh  : old_meshes) {
//    // extract meshlet separation info ------------------------------------------------
//    graphics::animated_meshlet_pack::meshlet new_meshlet;
//    std::unordered_map<uint32_t, uint32_t> id_map;
//    uint32_t current_id = 0;
//
//    // fill vertex info
//    for (const auto &v_kv: old_mesh->get_vertex_map()) {
//      auto &v = v_kv.second;
//      // register to id_map
//      id_map[v->id_] = current_id;
//      // add to meshlet
//      new_meshlet.vertex_indices[current_id++] = v->id_;
//    }
//    new_meshlet.vertex_count = current_id;
//
//    current_id = 0;
//    // fill primitive index info
//    for (const auto &f_kv: old_mesh->get_face_map()) {
//      auto &f = f_kv.second;
//      auto &he = f->half_edge_;
//      for (int i = 0; i < 3; i++) {
//        new_meshlet.primitive_indices[current_id++] = id_map[he->get_vertex()->id_];
//        he = he->get_next();
//      }
//    }
//    new_meshlet.index_count = current_id;
//
//    meshlet_pack.meshlets.emplace_back(std::move(new_meshlet));
//
//    // extract spheres --------------------------------------------------------------
//    const auto& spheres = old_mesh->get_bounding_volumes();
//
//    for (int i = 0; i < frame_count; i++) {
//      const auto& sphere = spheres[i];
//      auto center = sphere->get_local_center_point().cast<float>();
//      vec4 new_sphere = {
//        center.x(),
//        center.y(),
//        center.z(),
//        static_cast<float>(sphere->get_sphere_radius())
//      };
//
//      meshlet_pack.spheres[i].emplace_back(std::move(new_sphere));
//    }
//  }
//
//  return meshlet_pack;
//}
//
//// for easy benchmark
//std::vector<std::vector<s_ptr<mesh_model>>> separate_frame_greedy(const std::vector<s_ptr<separation_helper>>& helpers)
//{
//  std::vector<std::vector<s_ptr<mesh_model>>> frame_meshlets;
//
//  auto frame_count = helpers.size();
//  frame_meshlets.resize(frame_count);
//
//  auto start = std::chrono::system_clock::now();
//
//  // extract representative
//  auto& rep = helpers[0];
//  auto crtr = rep->get_criterion();
//  s_ptr<face> current_face = rep->get_random_remaining_face();
//  auto current_face_id = current_face->id_;
//
//  while (!rep->all_face_is_registered()) {
//    // compute each meshlet
//    std::vector<s_ptr<mesh_model>> mls;
//    mls.resize(frame_count);
//    for (int i = 0; i < frame_count; i++) {
//      mls[i] = mesh_model::create();
//    }
//
//    auto& rep_ml = mls[0];
//
//    // create bounding spheres for each frame
//    std::vector<u_ptr<bounding_volume>> bvs;
//    bvs.resize(frame_count);
//    for (int i = 0; i < frame_count; i++) {
//      bvs[i] = create_b_sphere_from_single_face(helpers[i]->get_face(current_face_id));
//    }
//
//    face_map adjoining_face_map {{current_face->id_ ,current_face}};
//
//    // meshlet api limitation
//    while (rep_ml->get_vertex_count() < graphics::meshlet_constants::MAX_VERTEX_COUNT
//           && rep_ml->get_face_count() < graphics::meshlet_constants::MAX_PRIMITIVE_COUNT
//           && !adjoining_face_map.empty()) {
//
//      current_face_id = choose_the_best_face_for_animation(adjoining_face_map, bvs, helpers, FRAME_SAMPLING_STRIDE);
//      current_face = rep->get_face(current_face_id);
//
//      // update each object
//      for (int i = 0; i < frame_count; i++) {
//        auto& ml = mls[i];
//        add_face_to_meshlet(helpers[i]->get_face(current_face_id), ml);
//      }
//
//      for (int i = 0; i < frame_count; i++) {
//        update_sphere(*bvs[i], helpers[i]->get_face(current_face_id));
//      }
//
//      rep->update_adjoining_face_map(adjoining_face_map, current_face);
//      rep->remove_face(current_face->id_);
//    }
//    // assign bv
//    for (int i = 0; i < frame_count; i++) {
//      mls[i]->set_bounding_volume(std::move(bvs[i]));
//    }
//    for (int i = 0; i < frame_count; i++) {
//      frame_meshlets[i].emplace_back(std::move(mls[i]));
//    }
//
//    current_face = choose_random_face_from_map(adjoining_face_map);
//    if (current_face == nullptr)
//      current_face = rep->get_random_remaining_face();
//  }
//
//  auto end = std::chrono::system_clock::now();
//
//  double time = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
//  printf("time %lf[m]\n", time);
//
//  return frame_meshlets;
//}

std::vector<graphics::meshlet> mesh_separation::separate(
  const s_ptr<mesh_model>& _model,
  const std::string& _model_name,
  criterion _crtr)
{
  std::vector<graphics::meshlet> meshlets;

  auto helper = separation_helper::create(_model, _model_name, _crtr);
  auto vertex_count = helper->get_vertex_map().size();

  auto geometry_meshlets = separate_greedy(helper);

  for (auto& meshlet : geometry_meshlets) {
    meshlets.emplace_back(translate_to_meshlet(meshlet));
  }

  write_meshlet_cache(meshlets, vertex_count, helper->get_model_name(), helper->get_criterion());

  return meshlets;
}

std::vector<s_ptr<mesh_model>> mesh_separation::separate_into_raw(
  const s_ptr<hnll::geometry::mesh_model> &_model,
  const std::string &model_name,
  hnll::geometry::mesh_separation::criterion crtr)
{
  auto helper = separation_helper::create(_model, model_name, crtr);

  auto geometry_meshlets = separate_greedy(helper);

  colorize_meshlets(geometry_meshlets);

  return geometry_meshlets;
}

//graphics::animated_meshlet_pack mesh_separation::separate_into_meshlet_pack(
//  const std::vector<s_ptr<mesh_model>>& _models,
//  criterion _crtr)
//{
//  graphics::animated_meshlet_pack meshlet_pack;
//
//  std::vector<s_ptr<separation_helper>> helpers;
//  for (const auto& model : _models) {
//    helpers.emplace_back(separation_helper::create(model, "", _crtr));
//  }
//  auto geometry_meshlets = separate_animation_greedy(helpers);
//
//  meshlet_pack = translate_to_animated_meshlet_pack(geometry_meshlets);
//
//  return meshlet_pack;
//}
//
//std::vector<std::vector<s_ptr<mesh_model>>> mesh_separation::separate_into_raw_frame(
//  const std::vector<s_ptr<mesh_model>> &_models,
//  hnll::geometry::mesh_separation::criterion _crtr)
//{
//  std::vector<s_ptr<separation_helper>> helpers;
//  for (const auto& model : _models) {
//    helpers.emplace_back(separation_helper::create(model, "", _crtr));
//  }
//  auto geometry_meshlets = separate_frame_greedy(helpers);
//
//  for (auto& meshlets : geometry_meshlets) {
//    colorize_meshlets(meshlets);
//  }
//
//  return geometry_meshlets;
//}

void mesh_separation::write_meshlet_cache(
  const std::vector<graphics::meshlet> &_meshlets,
  const size_t vertex_count,
  const std::string& _filename,
  criterion _crtr)
{
  auto directory = utils::create_sub_cache_directory("meshlets");

  std::ofstream writing_file;
  std::string filepath = directory + "/" + _filename + ".ml";
  writing_file.open(filepath, std::ios::out);

  // write contents
  writing_file << filepath << std::endl;
  writing_file << "greedy" << std::endl;
  switch (_crtr) {
    case criterion::MINIMIZE_BOUNDING_SPHERE :
      writing_file << "MINIMIZE_BOUNDING_SPHERE" << std::endl;
      break;
    case criterion::MINIMIZE_AABB :
      writing_file << "MINIMIZE_AABB" << std::endl;
      // temporary
      return;
      break;
    default :
      ;
  }

  writing_file << "vertex count" << std::endl;
  writing_file << vertex_count << std::endl;
  writing_file << "max vertex count per meshlet" << std::endl;
  writing_file << graphics::meshlet_constants::MAX_VERTEX_COUNT << std::endl;
  writing_file << "max primitive indices count per meshlet" << std::endl;
  writing_file << graphics::meshlet_constants::MAX_PRIMITIVE_INDICES_COUNT << std::endl;

  auto meshlet_count = _meshlets.size();
  writing_file << meshlet_count << std::endl;
  for (int i = 0; i < meshlet_count; i++) {
    auto current_ml = _meshlets[i];
    // vertex info
    writing_file << current_ml.vertex_count << std::endl;
    for (const auto& v_id : current_ml.vertex_indices) {
      writing_file << v_id << ",";
    }
    writing_file << std::endl;
    // face info
    writing_file << current_ml.index_count << std::endl;
    for (const auto& i_id : current_ml.primitive_indices) {
      writing_file << i_id << ",";
    }
    writing_file << std::endl;
    // bonding volume info
    writing_file << current_ml.center.x() << ',' <<
                 current_ml.center.y() << ',' <<
                 current_ml.center.z() << std::endl;
    writing_file << current_ml.radius << std::endl;
  }
  writing_file.close();
}

std::vector<graphics::meshlet> mesh_separation::load_meshlet_cache(const std::string &_filename)
{
  std::vector<graphics::meshlet> ret{};

  std::string cache_dir = std::string(getenv("HNLL_ENGN")) + "/cache/meshlets/";
  std::string file_path = cache_dir + _filename + ".ml";

  // cache does not exist
  if (!std::filesystem::exists(file_path)) {
    return ret;
  }

  std::ifstream reading_file(file_path);
  std::string buffer;

  if (reading_file.fail()) {
    throw std::runtime_error("failed to open file" + file_path);
  }

  // ignore first 4 lines
  for (int i = 0; i < 4; i++) {
    getline(reading_file, buffer);
  }
//
  getline(reading_file, buffer);
  getline(reading_file, buffer);
  getline(reading_file, buffer);
  int max_vertex_count = std::stoi(buffer);
  if (max_vertex_count != graphics::meshlet_constants::MAX_VERTEX_COUNT)
    return ret;
  getline(reading_file, buffer);
  getline(reading_file, buffer);
  int max_primitive_indices_count = std::stoi(buffer);
  if (max_primitive_indices_count != graphics::meshlet_constants::MAX_PRIMITIVE_INDICES_COUNT)
    return ret;

  // meshlet count
  getline(reading_file, buffer);
  uint32_t meshlet_count = std::stoi(buffer);
  ret.resize(meshlet_count);

  // read info
  for (int i = 0; i < meshlet_count; i++) {
    // vertex count
    getline(reading_file, buffer);
    auto vertex_count = std::stoi(buffer);
    ret[i].vertex_count = vertex_count;
    // vertex indices array
    for (int j = 0; j < vertex_count; j++) {
      getline(reading_file, buffer, ',');
      ret[i].vertex_indices[j] = std::stoi(buffer);
    }
    getline(reading_file, buffer);
    // index count
    getline(reading_file, buffer);
    auto index_count = std::stoi(buffer);
    ret[i].index_count = index_count;
    // primitive indices array
    for (int j = 0; j < index_count; j++) {
      getline(reading_file, buffer, ',');
      ret[i].primitive_indices[j] = std::stoi(buffer);
    }
    getline(reading_file, buffer);
    // bounding volume
    // center
    getline(reading_file, buffer, ',');
    ret[i].center.x() = std::stof(buffer);
    getline(reading_file, buffer, ',');
    ret[i].center.y() = std::stof(buffer);
    getline(reading_file, buffer);
    ret[i].center.z() = std::stof(buffer);
    // radius
    getline(reading_file, buffer);
    ret[i].radius = std::stof(buffer);
  }

  return ret;
}

} // namespace hnll::geometry