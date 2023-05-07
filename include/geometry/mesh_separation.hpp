#pragma once

// hnll
#include <geometry/primitives.hpp>
#include <geometry/bounding_volume.hpp>
#include <utils/common_alias.hpp>

// std
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

namespace hnll {

// forward declaration
namespace graphics {
struct mesh_builder;
struct meshlet;
struct animated_meshlet_pack;
}

namespace geometry {

// forward declaration
template <bv_type type>
class mesh_model;

// ---------------------------------------------------------------------------------------
struct mesh_info
{
  vertex_id_set v_ids;
  face_id_set   f_ids;
};

// ---------------------------------------------------------------------------------------
namespace mesh_separation {

std::vector<graphics::meshlet> separate_into_graphics_meshlet(
  const s_ptr<mesh_model>& _model,
  const std::string& _model_name = "tmp");

std::vector<s_ptr<mesh_model>> separate_into_geometry_mesh(
  const s_ptr<mesh_model>& _model,
  const std::string& model_name = "tmp");

graphics::animated_meshlet_pack separate_into_meshlet_pack(
  const std::vector<s_ptr<mesh_model>>& _models);

std::vector<std::vector<s_ptr<mesh_model>>> separate_into_raw_frame(
  const std::vector<s_ptr<mesh_model>>& _models);

// meshlet cache file format
/*
 * model path
 * separation strategy (greedy...)
 * cost function type (sphere, aabb...)
 * meshlet count
 * meshlet id (0 ~ meshlet count) :
 *   vertex count
 *   meshlet count
 *   vertex indices array (split by , )
 *   triangle indices array (split by , )
 */

void write_meshlet_cache(
  const std::vector<graphics::meshlet>& _meshlets,
  const size_t vertex_count,
  const std::string& _filename);

// returns true if the cache exists
std::vector<graphics::meshlet> load_meshlet_cache(const std::string& _filename);

} // namespace mesh_separation


class separation_helper
{
  public:
    static s_ptr<separation_helper> create(
      const std::string& _model_name
    );
    explicit separation_helper(const s_ptr<mesh_model>& model);

    // getter
    const vertex_map&  get_vertex_map()         const { return v_map_; }
    const face_map&    get_face_map()           const { return f_map_; }
    face_id     get_random_remaining_face_id();
    const face& get_face(face_id id) { return f_map_.at(id); }
    bool        all_face_is_registered() const { return remaining_face_ids_.empty(); }
    bool   face_is_remaining(face_id id) const { return remaining_face_ids_.find(id) != remaining_face_ids_.end(); }
    std::string get_model_name()         const { return model_name_; }

    // setter
    void remove_face(face_id id) { remaining_face_ids_.erase(id); }
    void update_adjoining_face_map(face_map& adjoining_face_map, const s_ptr<face>& fc);
    void set_model_name(const std::string& _model_name) { model_name_ = _model_name; }

  private:
    template <bv_type type>
    u_ptr<bounding_volume<type>> create_bv_from_single_face(const face& f);
    double compute_loss(const aabb&, face_id);
    double compute_loss(const b_sphere&, face_id);

    bv_type type_;

    s_ptr<vertex>         current_vertex_;
    const vertex_map&       v_map_;
    const face_map&         f_map_;
    const half_edge_id_map& he_map_;
    face_id_set remaining_face_ids_;

    // for cache
    std::string                model_name_;
};

// impl -------------------------------------------------------------------------
#define SEPHEL_API template <bv_type type>
SEPHEL_API u_ptr<bounding_volume<type>> separation_helper::create_bv_from_single_face(const face &f)
{
  std::vector<vec3d> vertices;
  const auto& first = he_map_.at(f.he_id);
  auto current_id = first.this_id;
  do {
    const auto& current = he_map_.at(current_id);
    vertices.push_back(v_map_.at(current.v_id).position);
    current_id = he_map_.at(current.next).this_id;
  } while (current_id != first.this_id);
  return bounding_volume<type>::create(vertices);
}

}} // namespace hnll::geometry