#pragma once

// hnll
#include <graphics/skinning_utils.hpp>
#include <utils/common_alias.hpp>

namespace hnll::graphics {

class skinning_mesh_model;

namespace frame_anim_utils {

constexpr float MAX_FPS = 30.f;

struct dynamic_attributes {
  alignas(16) vec3 position;
  alignas(16) vec3 normal;
};

struct common_attributes {
  vec2 uv0;
  vec2 uv1;
  vec4 color;
};

std::vector<frame_anim_utils::dynamic_attributes> extract_dynamic_attributes(
  skinning_mesh_model& original,
  const skinning_utils::builder& builder);

} // namespace frame_anim_utils
} // namespace hnll::graphics