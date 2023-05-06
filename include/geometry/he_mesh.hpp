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

// forward declaration
enum class bv_type;
template <bv_type type> class bounding_volume;

// half edge mesh
class he_mesh
{
  public:
    static s_ptr<he_mesh> create();
    static s_ptr<he_mesh> create_from_obj_file(const std::string& filename);
    // for frame_anim_meshlet_model
    static std::vector<s_ptr<he_mesh>> create_from_dynamic_attributes(
      const std::vector<std::vector<graphics::frame_anim_utils::dynamic_attributes>>& vertices,
      const std::vector<uint32_t>& indices);

    he_mesh();
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

    const aabb& get_aabb() const;
    const std::vector<u_ptr<aabb>>& get_aabbs() const;
    const b_sphere& get_b_sphere() const;
    const std::vector<u_ptr<b_sphere>>& get_b_spheres() const;

    u_ptr<aabb> get_aabb_copy() const;
    u_ptr<aabb> move_aabb();
    u_ptr<b_sphere> get_b_sphere_copy() const;
    u_ptr<b_sphere> move_b_sphere();

    // for test
    bool exist_half_edge(half_edge_id id) { return half_edge_id_map_.find(id) != half_edge_id_map_.end(); }
    bool exist_half_edge(const vertex& v0, const vertex& v1);

    std::vector<graphics::vertex> move_raw_vertices() { return std::move(raw_vertices_); }

    // setter
    void set_aabb(u_ptr<aabb>&& bv);
    void set_aabbs(std::vector<u_ptr<aabb>>&& bvs);
    void set_b_sphere(u_ptr<b_sphere>&& bv);
    void set_b_spheres(std::vector<u_ptr<b_sphere>>&& bvs);

    void colorize_whole_mesh(const vec3& color);
  private:
    // returns false if the pair have not been registered to the map
    bool associate_half_edge_pair(half_edge& he);

    // geometric primitives
    half_edge_map half_edge_map_;
    half_edge_id_map half_edge_id_map_;
    face_map      face_map_;
    vertex_map    vertex_map_;

    // bounding_volumes
    u_ptr<aabb>     aabb_;
    u_ptr<b_sphere> b_sphere_;
    // for animation separation
    std::vector<u_ptr<aabb>> aabbs_;
    std::vector<u_ptr<b_sphere>> b_spheres_;

    // move to graphics::meshlet_model
    std::vector<graphics::vertex> raw_vertices_;

    uint32_t v_count_;
    uint32_t f_count_;
    uint32_t he_count_;
};



} // namespace hnll::geometry