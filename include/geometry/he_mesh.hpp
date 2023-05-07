#pragma once

// hnll
#include <geometry/primitives.hpp>
#include <geometry/bounding_volume.hpp>
#include <graphics/utils.hpp>

// hash for half edge map
namespace std {
template<>
struct hash<std::pair<hnll::geometry::vertex, hnll::geometry::vertex>>
{
  size_t operator() (const std::pair<hnll::geometry::vertex, hnll::geometry::vertex>& vertex_pair) const
  {
    size_t seed = 0;
    // only positions of vertex matter for half edge
    hnll::graphics::hash_combine(seed, vertex_pair.first.position, vertex_pair.second.position, vertex_pair.first.normal, vertex_pair.second.normal);
    return seed;
  }
};

} // namespace std

// forward declaration
namespace hnll::graphics
{
  class mesh_model;
  struct vertex;
  struct mesh_builder;
  namespace frame_anim_utils { struct dynamic_attributes; }
}

namespace hnll::geometry {

class he_mesh
{
  public:
    static s_ptr<he_mesh> create();
    static s_ptr<he_mesh> create_from_obj_file(const std::string& filename);
    // for frame_anim_meshlet_model
    static std::vector<s_ptr<he_mesh>> create_from_dynamic_attributes(
      const std::vector<std::vector<graphics::frame_anim_utils::dynamic_attributes>>& vertices,
      const std::vector<uint32_t>& indices);

    he_mesh() = default;
    void align_vertex_id();

    // vertices are assumed to be in a counter-clockwise order
    vertex_id add_vertex(vertex& v);
    face_id   add_face(vertex& v0, vertex& v1, vertex& v2, face_id id);

    // getter
    vertex_map       get_vertex_map() const         { return vertex_map_; }
    face_map         get_face_map() const           { return face_map_; }
    half_edge_map    get_half_edge_map() const      { return half_edge_map_; }
    size_t           get_face_count() const         { return face_map_.size(); }
    size_t           get_vertex_count() const       { return vertex_map_.size(); }
    size_t           get_half_edge_count() const    { return half_edge_map_.size(); }
    face&            get_face_r(face_id id)         { return face_map_.at(id); }
    vertex&          get_vertex_r(vertex_id id)     { return vertex_map_.at(id); }
    half_edge&       get_half_edge_r(const vertex& v0, const vertex& v1);
    half_edge&       get_half_edge_r(half_edge_id id) { return half_edge_id_map_.at(id); }

    const face&      get_face(face_id id)     const { return face_map_.at(id); }
    const vertex&    get_vertex(vertex_id id) const { return vertex_map_.at(id); }
    const half_edge& get_half_edge(half_edge_id id)  const { return half_edge_id_map_.at(id); }

    // for test
    bool exist_half_edge(half_edge_id id) { return half_edge_id_map_.find(id) != half_edge_id_map_.end(); }
    bool exist_half_edge(const vertex& v0, const vertex& v1);

    std::vector<graphics::vertex> move_raw_vertices() { return std::move(raw_vertices_); }

    // setter
    void colorize_whole_mesh(const vec3d& color);
  private:
    // returns false if the pair have not been registered to the map
    bool associate_half_edge_pair(half_edge& he);

    // geometric primitives
    half_edge_map half_edge_map_;
    half_edge_id_map half_edge_id_map_;
    face_map      face_map_;
    vertex_map    vertex_map_;

    // move to graphics::meshlet_model
    std::vector<graphics::vertex> raw_vertices_;
};

template <bv_type type>
class bv_mesh
{
  public:
    static u_ptr<bv_mesh<type>> create() { return std::make_unique<bv_mesh<type>>(); }

    // getter
    const bounding_volume<type>&                     get_bv() const { return *bv_; }
    const std::vector<u_ptr<bounding_volume<type>>>& get_bvs() const { return bvs_; }
    u_ptr<bounding_volume<type>> move_bv() { return std::move(bv_); }
    u_ptr<bounding_volume<type>> get_bv_copy() const
    { auto bv = *bv_; return std::make_unique<bounding_volume<type>>(bv); }

    // setter
    void set_bv(u_ptr<bounding_volume<type>>&& bv) { bv_ = std::move(bv); }
    void set_bvs(std::vector<u_ptr<bounding_volume<type>>>&& bvs) { bvs_ = std::move(bvs); }

    void add_v_id(vertex_id id) { v_ids_.emplace(id); }
    void add_f_id(face_id id)   { f_ids_.emplace(id); }

  private:
    vertex_id_set v_ids_;
    face_id_set   f_ids_;
    u_ptr<bounding_volume<type>> bv_;
    std::vector<u_ptr<bounding_volume<type>>> bvs_;
};

} // namespace hnll::geometry