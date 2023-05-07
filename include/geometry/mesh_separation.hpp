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
struct meshletBS;
struct animated_meshlet_pack;
}

namespace geometry {

class he_mesh;

// ---------------------------------------------------------------------------------------
namespace mesh_separation {

std::vector<graphics::meshletBS> separate_meshletBS(const he_mesh& original, const std::string& name);

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