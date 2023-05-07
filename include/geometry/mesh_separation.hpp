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

}} // namespace hnll::geometry