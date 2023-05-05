#pragma once

// hnll
#include <geometry/primitives.hpp>
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
class  bounding_volume;
enum class bv_type;

class mesh_model
{
  public:
    static s_ptr<mesh_model> create();
    static s_ptr<mesh_model> create_from_obj_file(const std::string& filename);
    // for frame_anim_meshlet_model
    static std::vector<s_ptr<mesh_model>> create_from_dynamic_attributes(
      const std::vector<std::vector<graphics::frame_anim_utils::dynamic_attributes>>& vertices,
      const std::vector<uint32_t>& indices);

    mesh_model();
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
    const bounding_volume& get_bounding_volume() const;
    const std::vector<u_ptr<bounding_volume>>& get_bounding_volumes() const;
    u_ptr<bounding_volume> get_bounding_volume_copy() const;
    u_ptr<bounding_volume> get_ownership_of_bounding_volume();

    // for test
    bool exist_half_edge(half_edge_id id) { return half_edge_id_map_.find(id) != half_edge_id_map_.end(); }
    bool exist_half_edge(const vertex& v0, const vertex& v1);

    std::vector<graphics::vertex> move_raw_vertices() { return std::move(raw_vertices_); }

    // setter
    void set_bounding_volume(u_ptr<bounding_volume>&& bv);
    void set_bounding_volumes(std::vector<u_ptr<bounding_volume>>&& bvs);
    void colorize_whole_mesh(const vec3& color);
    void set_bv_type(bv_type type);
  private:
    // returns false if the pair have not been registered to the map
    bool associate_half_edge_pair(half_edge& he);

    half_edge_map half_edge_map_;
    half_edge_id_map half_edge_id_map_;
    face_map      face_map_;
    vertex_map    vertex_map_;
    u_ptr<bounding_volume> bounding_volume_;
    // for animation separation
    std::vector<u_ptr<bounding_volume>> bounding_volumes_;
    // move to graphics::meshlet_model
    std::vector<graphics::vertex> raw_vertices_;

    uint32_t v_count_;
    uint32_t f_count_;
    uint32_t he_count_;
};



} // namespace hnll::geometry