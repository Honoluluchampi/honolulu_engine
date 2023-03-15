#pragma once

// hnll
#include <utils/common_alias.hpp>

namespace hnll::graphics {

// use same meshlet constants as mesh shaders
namespace meshlet_constants {
#include "../../modules/graphics/shaders/mesh_shader/meshlet_constants.h"
}

constexpr uint32_t VERTEX_DESC_ID  = 0;
constexpr uint32_t MESHLET_DESC_ID = 1;
constexpr uint32_t DESC_SET_COUNT  = 2;

struct meshlet
{
  uint32_t vertex_indices   [meshlet_constants::MAX_VERTEX_COUNT]; // indicates position in a vertex buffer
  uint32_t primitive_indices[meshlet_constants::MAX_PRIMITIVE_INDICES_COUNT];
  uint32_t vertex_count;
  uint32_t index_count;
  // for frustum culling (for bounding sphere)
  alignas(16) vec3 center;
  float            radius;
  // for aabb
  // alignas(16) vec3 radius;
};

struct animated_meshlet_pack
{
  struct meshlet
  {
    uint32_t vertex_indices   [meshlet_constants::MAX_VERTEX_COUNT]; // indicates position in a vertex buffer
    uint32_t primitive_indices[meshlet_constants::MAX_PRIMITIVE_INDICES_COUNT];
    uint32_t vertex_count;
    uint32_t index_count;
  };

  std::vector<animated_meshlet_pack::meshlet> meshlets;
  std::vector<std::vector<vec4>> spheres;
};

} // namespace hnll::graphics