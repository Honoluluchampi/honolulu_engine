#pragma once

// hnll
#include <utils/common_alias.hpp>

// lib
#include <vulkan/vulkan.h>

#define MAX_LIGHTS 20

namespace hnll::utils {

constexpr int FRAMES_IN_FLIGHT = 2;

enum class rendering_type
{
  VERTEX_SHADING,
  RAY_TRACING,
  MESH_SHADING
};

// rendering order matters for alpha blending
// solid object should be drawn first, then transparent object should be drawn after that
enum class shading_type : uint32_t
{
  MESH               = 100,
  SKINNING_MESH      = 101,
  FRAME_ANIM_MESH    = 102,
  CLOTH              = 103,
  MESHLET            = 200,
  FRAME_ANIM_MESHLET = 201,
  POINT_LIGHT  = 400,
  LINE         = 0,
  UNIQUE       = 1,
  WIRE_FRUSTUM = 300,
  GRID         = 500
};

struct point_light
{
  vec4 position{}; // ignore w
  vec4 color{}; // w is intensity
};

// global uniform buffer object
struct global_ubo
{
  // check alignment rules
  mat4 projection   = mat4::Identity();
  mat4 view         = mat4::Identity();
  mat4 inverse_view = mat4::Identity();
  // point light
  vec4 ambient_light_color{1.f, 1.f, 1.f, .02f}; // w is light intensity
  point_light point_lights[MAX_LIGHTS];
  int lights_count;
};

struct viewer_info
{
  mat4 projection   = mat4::Identity();
  mat4 view         = mat4::Identity();
  mat4 inverse_view = mat4::Identity();
};

struct frustum_info
{
  alignas(16) vec3 camera_position;
  alignas(16) vec3 near_position;
  alignas(16) vec3 far_position;
  alignas(16) vec3 top_n;
  alignas(16) vec3 bottom_n;
  alignas(16) vec3 right_n;
  alignas(16) vec3 left_n;
  alignas(16) vec3 near_n;
  alignas(16) vec3 far_n;
};
} // namespace hnll::utils