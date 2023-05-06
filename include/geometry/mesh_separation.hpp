#pragma once

// hnll
#include <geometry/bounding_volume.hpp>
#include <utils/common_alias.hpp>

// std
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <set>

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
class vertex;
class face;

using vertex_id  = uint32_t;
using vertex_map = std::unordered_map<vertex_id, s_ptr<vertex>>;
using face_id    = uint32_t;
using face_map   = std::unordered_map<face_id, s_ptr<face>>;
using remaining_face_id_set = std::set<face_id>;

namespace mesh_separation {

enum class solution {
  GREEDY,
  K_MEANS_BASED
};

enum class criterion {
  MINIMIZE_AABB,
  MINIMIZE_BOUNDING_SPHERE
};

std::vector<graphics::meshlet> separate_into_graphics_meshlet(
  const s_ptr<mesh_model>& _model,
  const std::string& _model_name = "tmp",
  criterion _crtr = criterion::MINIMIZE_BOUNDING_SPHERE);

std::vector<s_ptr<mesh_model>> separate_into_geometry_mesh(
  const s_ptr<mesh_model>& _model,
  const std::string& model_name = "tmp",
  criterion crtr = criterion::MINIMIZE_BOUNDING_SPHERE);

graphics::animated_meshlet_pack separate_into_meshlet_pack(
  const std::vector<s_ptr<mesh_model>>& _models,
  criterion _crtr = criterion::MINIMIZE_BOUNDING_SPHERE);

std::vector<std::vector<s_ptr<mesh_model>>> separate_into_raw_frame(
  const std::vector<s_ptr<mesh_model>>& _models,
  criterion _crtr = criterion::MINIMIZE_BOUNDING_SPHERE);

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
  const std::string& _filename,
  criterion _crtr);

// returns true if the cache exists
std::vector<graphics::meshlet> load_meshlet_cache(const std::string& _filename);

} // namespace mesh_separation

using criterion_map = std::unordered_map<mesh_separation::criterion, std::function<double()>>;
using solution_map  = std::unordered_map<mesh_separation::solution, std::function<s_ptr<mesh_model>()>>;

class mesh_separation_helper
{
  public:
    static s_ptr<mesh_separation_helper> create(
      const s_ptr<mesh_model>& model,
      const std::string& _model_name,
      mesh_separation::criterion criterion
    );
    explicit mesh_separation_helper(const s_ptr<mesh_model>& model);

    void   compute_whole_shape_diameters();

    std::vector<mesh_model> separate_using_sdf();

    // getter
    vertex_map  get_vertex_map()         const { return vertex_map_; }
    face_map    get_face_map()           const { return face_map_; }
    s_ptr<face> get_random_remaining_face();
    const s_ptr<face>& get_face(face_id id) { return face_map_[id]; }
    bool        all_face_is_registered() const { return remaining_face_id_set_.empty(); }
    bool        vertex_is_empty()        const { return vertex_map_.empty(); }
    bool   face_is_remaining(face_id id) const { return remaining_face_id_set_.find(id) != remaining_face_id_set_.end(); }
    std::string get_model_name()         const { return model_name_; }
    mesh_separation::criterion get_criterion() { return criterion_; }

    // setter
    void remove_face(face_id id) { remaining_face_id_set_.erase(id); }
    void update_adjoining_face_map(face_map& adjoining_face_map, const s_ptr<face>& fc);
    void set_criterion(mesh_separation::criterion crtr) { criterion_ = crtr; }
    void set_model_name(const std::string& _model_name) { model_name_ = _model_name; }

  private:
    s_ptr<mesh_model>     model_;

    s_ptr<vertex>         current_vertex_;
    vertex_map            vertex_map_;
    face_map              face_map_;
    remaining_face_id_set remaining_face_id_set_;

    static criterion_map  criterion_map_;
    static solution_map   solution_map_;

    // for cache
    mesh_separation::criterion criterion_;
    std::string                model_name_;
};

}} // namespace hnll::geometry