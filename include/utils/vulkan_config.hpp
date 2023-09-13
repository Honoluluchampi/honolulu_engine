#pragma once

namespace hnll::utils {

constexpr int FRAMES_IN_FLIGHT = 3;

enum class rendering_type
{
  VERTEX_SHADING,
  RAY_TRACING,
  MESH_SHADING
};

enum class present_mode
{
    IMMEDIATE,
    V_SYNC
};

struct vulkan_config
{
  rendering_type rendering = rendering_type::VERTEX_SHADING;
  present_mode present = present_mode::V_SYNC;
  bool enable_validation_layers = true; // for debug build
};

}