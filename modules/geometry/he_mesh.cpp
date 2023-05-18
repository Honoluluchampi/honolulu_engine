// hnll
#include <geometry/he_mesh.hpp>
#include <geometry/primitives.hpp>
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

s_ptr<he_mesh> he_mesh::create()
{ return std::make_shared<he_mesh>(); }

half_edge& he_mesh::get_half_edge_r(const vertex& v0, const vertex& v1)
{
  half_edge_key hash_key = { v0, v1 };
  return half_edge_map_.at(hash_key);
}

vertex translate_vertex_graphics_to_geometry(const graphics::vertex& pseudo, vertex_id id)
{
  vertex v {pseudo.position.cast<double>(), id};
  v.color    = pseudo.color.cast<double>();
  v.normal   = pseudo.normal.cast<double>();
  v.uv       = pseudo.uv.cast<double>();
  return v;
}

s_ptr<he_mesh> he_mesh::create_from_obj_file(const std::string& filename)
{
  auto mesh_model = he_mesh::create();

  mesh_model->raw_vertices_.clear();

  // load data
  graphics::obj_loader loader;
  loader.load_model(filename);

  // assign to mesh model
  for (int i = 0; i < loader.vertices.size(); i++) {
    auto& gra_v = loader.vertices[i];
    auto geo_v = translate_vertex_graphics_to_geometry(gra_v, mesh_model->get_vertex_count());
    mesh_model->add_vertex(geo_v);
    mesh_model->raw_vertices_.emplace_back(std::move(gra_v));
  }

  std::vector<vertex_id> indices = std::move(loader.indices);

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
  vertex v { pseudo.position.cast<double>(), id};
  v.normal = pseudo.normal.cast<double>();
  return v;
}

std::vector<s_ptr<he_mesh>> he_mesh::create_from_dynamic_attributes(
  const std::vector<std::vector<graphics::frame_anim_utils::dynamic_attributes>> &vertices_pack,
  const std::vector<uint32_t> &indices)
{
  std::vector<s_ptr<he_mesh>> models;

  if (indices.size() % 3 != 0)
    throw std::runtime_error("index count is not multiple of 3.");

  for (const auto& vertices : vertices_pack) {
    auto model = he_mesh::create();

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

void he_mesh::align_vertex_id()
{
  vertex_map new_map;
  vertex_id new_id = 0;
  for (auto& kv : vertex_map_) {
    kv.second.v_id = new_id;
    new_map.at(new_id++) = kv.second;
  }
  vertex_map_ = new_map;
}

void he_mesh::colorize_whole_mesh(const vec3d& color)
{ for (auto& kv : face_map_) kv.second.color = color; }


// assumes that he.next has already assigned
bool he_mesh::associate_half_edge_pair(half_edge &he)
{
  auto& start_v = vertex_map_.at(he.v_id);
  auto end_v_id = half_edge_id_map_.at(he.next).v_id;
  auto& end_v = vertex_map_.at(end_v_id);

  half_edge_key hash_key = { start_v, end_v };
  half_edge_key pair_hash_key = { end_v, start_v };

  // register this half edge
  half_edge_map_.emplace(hash_key, he);

  // check if those hash_key already have a value
  if (half_edge_map_.find(pair_hash_key) != half_edge_map_.end()) {
    // if the pair has added to the map, associate with it
    half_edge_map_.at(hash_key).pair      = half_edge_map_.at(pair_hash_key).this_id;
    half_edge_map_.at(pair_hash_key).pair = he.this_id;

    return true;
  }
  return false;
}

vertex_id he_mesh::add_vertex(vertex &v)
{
  // if the vertex has not been involved
  if (vertex_map_.count(v.v_id) == 0) {
    vertex_map_.emplace(v.v_id, v);
  }
  return v.v_id;
}
face_id he_mesh::add_face(vertex& v0, vertex& v1, vertex& v2, face_id id)
{
  // register to the vertex hash table
  add_vertex(v0);
  add_vertex(v1);
  add_vertex(v2);

  half_edge_id current_he_count = get_half_edge_count();
  // create new half_edge
  std::array<half_edge, 3> hes = {
    half_edge{ v0, current_he_count },
    half_edge{ v1, current_he_count + 1 },
    half_edge{ v2, current_he_count + 2 }
  };

  // half_edge circle
  hes[0].next = hes[1].this_id; hes[0].prev = hes[2].this_id;
  hes[1].next = hes[2].this_id; hes[1].prev = hes[0].this_id;
  hes[2].next = hes[0].this_id; hes[2].prev = hes[1].this_id;

  // new face
  face fc { id, hes[0].this_id };

  // calc face normal
  fc.normal = ((v1.position - v0.position).cross(v2.position - v0.position)).normalized();

  // register to each owner
  face_map_.emplace(fc.f_id, fc);
  hes[0].f_id = fc.f_id;
  hes[1].f_id = fc.f_id;
  hes[2].f_id = fc.f_id;

  get_vertex_r(v0.v_id).face_count++;
  get_vertex_r(v1.v_id).face_count++;
  get_vertex_r(v2.v_id).face_count++;

  // assign to id map
  half_edge_id_map_.emplace(hes[0].this_id, hes[0]);
  half_edge_id_map_.emplace(hes[1].this_id, hes[1]);
  half_edge_id_map_.emplace(hes[2].this_id, hes[2]);

  // register to the hash_table
  for (int i = 0; i < 3; i++) {
    associate_half_edge_pair(half_edge_id_map_.at(hes[i].this_id));
  }

  return fc.f_id;
}

bool he_mesh::exist_half_edge(const vertex& v0, const vertex& v1)
{
  half_edge_key hash_key = { v0, v1 };
  return half_edge_map_.find(hash_key) != half_edge_map_.end();
}

} // namespace hnll::geometry