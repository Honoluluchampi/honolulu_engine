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
  return (rhs.position_ == lhs.position_)
         && (rhs.color_    == lhs.color_)
         && (rhs.normal_   == lhs.normal_)
         && (rhs.uv_       == lhs.uv_);
}

s_ptr<mesh_model> mesh_model::create()
{ return std::make_shared<mesh_model>(); }

mesh_model::mesh_model() { bounding_volume_ = bounding_volume::create_blank_aabb(); }

s_ptr<half_edge> mesh_model::get_half_edge(const s_ptr<hnll::geometry::vertex> &v0, const s_ptr<hnll::geometry::vertex> &v1)
{
  half_edge_key hash_key = { *v0, *v1 };
  return half_edge_map_[hash_key];
}

s_ptr<vertex> translate_vertex_graphics_to_geometry(const graphics::vertex& pseudo)
{
  auto vertex_sp = vertex::create(pseudo.position);
  vertex_sp->color_    = pseudo.color;
  vertex_sp->normal_   = pseudo.normal;
  vertex_sp->uv_       = pseudo.uv;
  return vertex_sp;
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
  for (int i = 0; i < indices.size(); i += 3) {
    auto v0 = mesh_model->get_vertex(indices[i]);
    auto v1 = mesh_model->get_vertex(indices[i + 1]);
    auto v2 = mesh_model->get_vertex(indices[i + 2]);
    mesh_model->add_face(v0, v1, v2);
  }

  return mesh_model;
}

s_ptr<vertex> translate_vertex(const graphics::frame_anim_utils::dynamic_attributes& pseudo, uint32_t id)
{
  auto vertex_sp = vertex::create(pseudo.position);
  vertex_sp->normal_   = pseudo.normal;
  vertex_sp->id_ = id;
  return vertex_sp;
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
      auto v0 = model->get_vertex(indices[i]);
      auto v1 = model->get_vertex(indices[i + 1]);
      auto v2 = model->get_vertex(indices[i + 2]);
      model->add_face(v0, v1, v2, f_id++, false);
    }
    models.emplace_back(std::move(model));
  }
  return models;
}

void mesh_model::align_vertex_id()
{
  vertex_map new_map;
  vertex_id new_id = 0;
  for (const auto& kv : vertex_map_) {
    kv.second->id_ = new_id;
    new_map[new_id++] = kv.second;
  }
  vertex_map_ = new_map;
}

void mesh_model::colorize_whole_mesh(const vec3& color)
{ for (const auto& kv : face_map_) kv.second->color_ = color; }

bool mesh_model::associate_half_edge_pair(const s_ptr<half_edge> &he)
{
  half_edge_key hash_key = { *he->get_vertex(), *he->get_next()->get_vertex() };
  half_edge_key pair_hash_key = { *he->get_next()->get_vertex(), *he->get_vertex() };

  half_edge_map_[hash_key] = he;
  // check if those hash_key already have a value
  if (half_edge_map_.find(pair_hash_key) != half_edge_map_.end()) {
    // if the pair has added to the map, associate with it
    he->set_pair(half_edge_map_[pair_hash_key]);
    half_edge_map_[pair_hash_key]->set_pair(he);
    return true;
  }
  return false;
}

vertex_id mesh_model::add_vertex(const s_ptr<vertex> &v)
{
  // if the vertex has not been involved
  if (vertex_map_.count(v->id_) == 0) {
    if (v->id_ == -1)
      v->id_ = raw_vertices_.size();
    vertex_map_[v->id_] = v;
  }
  return v->id_;
}

face_id mesh_model::add_face(s_ptr<vertex> &v0, s_ptr<vertex> &v1, s_ptr<vertex> &v2, face_id id, bool auto_id, auto_vertex_normal_calculation avnc)
{
  // register to the vertex hash table
  add_vertex(v0);
  add_vertex(v1);
  add_vertex(v2);
  // create new half_edge
  std::array<s_ptr<half_edge>, 3> hes;
  hes[0] = half_edge::create(v0);
  hes[1] = half_edge::create(v1);
  hes[2] = half_edge::create(v2);
  // half_edge circle
  hes[0]->set_next(hes[1]); hes[0]->set_prev(hes[2]);
  hes[1]->set_next(hes[2]); hes[0]->set_prev(hes[0]);
  hes[2]->set_next(hes[0]); hes[0]->set_prev(hes[1]);
  // register to the hash_table
  for (int i = 0; i < 3; i++) {
    associate_half_edge_pair(hes[i]);
  }
  // new face
  auto fc = face::create(hes[0]);
  if (!auto_id)
    fc->id_ = id;
  // calc face normal
  fc->normal_ = ((v1->position_ - v0->position_).cross(v2->position_ - v0->position_)).normalized();
  // register to each owner
  face_map_[fc->id_] = fc;
  hes[0]->set_face(fc);
  hes[1]->set_face(fc);
  hes[2]->set_face(fc);
  // vertex normal
  if (avnc == auto_vertex_normal_calculation::ON) {
    v0->update_normal(fc->normal_);
    v1->update_normal(fc->normal_);
    v2->update_normal(fc->normal_);
  }
  else {
    v0->face_count_++;
    v1->face_count_++;
    v2->face_count_++;
  }
  return fc->id_;
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