// hnll
#include <geometry/mesh_model.hpp>
#include <geometry/primitives.hpp>
#include <geometry/bounding_volume.hpp>
#include <graphics/utils.hpp>
#include <graphics/frame_anim_utils.hpp>

// std
#include <filesystem>

// lib
#include <tiny_obj_loader/tiny_obj_loader.h>

namespace hnll::geometry {

bool operator==(const vertex& rhs, const vertex& lhs)
{
  return (rhs.position == lhs.position)
         && (rhs.color    == lhs.color)
         && (rhs.normal   == lhs.normal)
         && (rhs.uv       == lhs.uv);
}

s_ptr<mesh_model> mesh_model::create()
{ return std::make_shared<mesh_model>(); }

mesh_model::mesh_model() { bounding_volume_ = bounding_volume::create_blank_aabb(); }

half_edge& mesh_model::get_half_edge_r(const vertex& v0, const vertex& v1)
{
  half_edge_key hash_key = { v0, v1 };
  return half_edge_map_.at(hash_key);
}

vertex translate_vertex_graphics_to_geometry(const graphics::vertex& pseudo)
{
  vertex v {pseudo.position};
  v.color    = pseudo.color;
  v.normal   = pseudo.normal;
  v.uv       = pseudo.uv;
  return v;
}

s_ptr<mesh_model> mesh_model::create_from_obj_file(const std::string& filename)
{
  auto mesh_model = mesh_model::create();

  mesh_model->raw_vertices_.clear();
  std::vector<vertex_id> indices{};

  // load data
  graphics::obj_loader loader;
  loader.load_model(filename);

  // assign to mesh model
  for (int i = 0; i < loader.vertices.size(); i++) {
    auto& gra_v = loader.vertices[i];
    auto geo_v = translate_vertex_graphics_to_geometry(gra_v);
    mesh_model->add_vertex(geo_v);
    mesh_model->raw_vertices_.emplace_back(std::move(gra_v));
  }
  indices = std::move(loader.indices);

  if (indices.size() % 3 != 0)
    throw std::runtime_error("vertex count is not multiple of 3");

  // recreate all faces
  face_id f_id = 0;
  for (int i = 0; i < indices.size(); i += 3) {
    auto& v0 = mesh_model->get_vertex_r(indices[i]);
    auto& v1 = mesh_model->get_vertex_r(indices[i + 1]);
    auto& v2 = mesh_model->get_vertex_r(indices[i + 2]);
    mesh_model->add_face(v0, v1, v2, f_id++);
  }

  return mesh_model;
}

vertex translate_vertex(const graphics::frame_anim_utils::dynamic_attributes& pseudo, uint32_t id)
{
  vertex v { pseudo.position };
  v.normal = pseudo.normal;
  v.v_id   = id;
  return v;
}

std::vector<s_ptr<mesh_model>> mesh_model::create_from_dynamic_attributes(
  const std::vector<std::vector<graphics::frame_anim_utils::dynamic_attributes>> &vertices_pack,
  const std::vector<uint32_t> &indices)
{
  std::vector<s_ptr<mesh_model>> models;

  if (indices.size() % 3 != 0)
    throw std::runtime_error("index count is not multiple of 3.");

  for (const auto& vertices : vertices_pack) {
    auto model = mesh_model::create();

    // translate vertices
    for (int i = 0; i < vertices.size(); i++) {
      auto new_vertex = translate_vertex(vertices[i], i);
      model->add_vertex(new_vertex);
    }

    // recreate all faces
    face_id f_id = 0;
    for (int i = 0; i < indices.size(); i += 3) {
      auto v0 = model->get_vertex_r(indices[i]);
      auto v1 = model->get_vertex_r(indices[i + 1]);
      auto v2 = model->get_vertex_r(indices[i + 2]);
      model->add_face(v0, v1, v2, f_id++);
    }
    models.emplace_back(std::move(model));
  }
  return models;
}

void mesh_model::align_vertex_id()
{
  vertex_map new_map;
  vertex_id new_id = 0;
  for (auto& kv : vertex_map_) {
    kv.second.v_id = new_id;
    new_map.at(new_id++) = kv.second;
  }
  vertex_map_ = new_map;
}

void mesh_model::colorize_whole_mesh(const vec3& color)
{ for (auto& kv : face_map_) kv.second.color = color; }


// assumes that he.next has already assigned
bool mesh_model::associate_half_edge_pair(half_edge &he)
{
  auto& start_v = vertex_map_.at(he.v_id);
  auto end_v_id = half_edge_id_map_.at(he.next).v_id;
  auto& end_v = vertex_map_.at(end_v_id);

  half_edge_key hash_key = { start_v, end_v };
  half_edge_key pair_hash_key = { end_v, start_v };

  // register this half edge
  half_edge_map_.at(hash_key) = he;

  // check if those hash_key already have a value
  if (half_edge_map_.find(pair_hash_key) != half_edge_map_.end()) {
    // if the pair has added to the map, associate with it
    he.pair = half_edge_map_.at(pair_hash_key).this_id;
    half_edge_map_.at(pair_hash_key).this_id = he.this_id;
    return true;
  }
  return false;
}

vertex_id mesh_model::add_vertex(vertex &v)
{
  // if the vertex has not been involved
  if (vertex_map_.count(v.v_id) == 0) {
    // if vertex id is not assigned
    if (v.v_id == -1)
      v.v_id = raw_vertices_.size();
    vertex_map_.at(v.v_id) = v;
  }
  return v.v_id;
}
face_id mesh_model::add_face(vertex& v0, vertex& v1, vertex& v2, face_id id)
{
  // register to the vertex hash table
  add_vertex(v0);
  add_vertex(v1);
  add_vertex(v2);

  // create new half_edge
  std::array<half_edge, 3> hes = { v0, v1, v2 };

  // half_edge circle
  hes[0].next = hes[1].this_id; hes[0].prev = hes[2].this_id;
  hes[1].next = hes[2].this_id; hes[0].prev = hes[0].this_id;
  hes[2].next = hes[0].this_id; hes[0].prev = hes[1].this_id;

  // assign to id map
  half_edge_id_map_.at(hes[0].this_id) = hes[0];
  half_edge_id_map_.at(hes[1].this_id) = hes[1];
  half_edge_id_map_.at(hes[2].this_id) = hes[2];

  // register to the hash_table
  for (int i = 0; i < 3; i++) {
    associate_half_edge_pair(hes[i]);
  }

  // new face
  face fc { id, hes[0].this_id };

  // calc face normal
  fc.normal = ((v1.position - v0.position).cross(v2.position - v0.position)).normalized();

  // register to each owner
  face_map_[fc.f_id] = fc;
  hes[0].f_id = fc.f_id;
  hes[1].f_id = fc.f_id;
  hes[2].f_id = fc.f_id;

  v0.face_count++;
  v1.face_count++;
  v2.face_count++;

  return fc.f_id;
}

const bounding_volume& mesh_model::get_bounding_volume() const
{ return *bounding_volume_; }

const std::vector<u_ptr<bounding_volume>>& mesh_model::get_bounding_volumes() const
{ return bounding_volumes_; }

u_ptr<bounding_volume> mesh_model::get_ownership_of_bounding_volume()
{ return std::move(bounding_volume_); }

u_ptr<bounding_volume> mesh_model::get_bounding_volume_copy() const
{
  auto res = bounding_volume::create_blank_aabb();
  res->set_bv_type(bounding_volume_->get_bv_type());
  res->set_center_point(bounding_volume_->get_local_center_point());
  res->set_aabb_radius(bounding_volume_->get_aabb_radius());
  return res;
}

void mesh_model::set_bounding_volume(u_ptr<bounding_volume> &&bv)
{ bounding_volume_ = std::move(bv); }

void mesh_model::set_bounding_volumes(std::vector<u_ptr<bounding_volume>> &&bvs)
{ bounding_volumes_ = std::move(bvs); }

void mesh_model::set_bv_type(bv_type type)
{ bounding_volume_->set_bv_type(type); }

} // namespace hnll::geometry