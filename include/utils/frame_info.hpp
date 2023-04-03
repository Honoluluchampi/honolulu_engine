#pragma once

#include <utils/rendering_utils.hpp>

namespace hnll::utils {

struct game_frame_info {
  uint64_t compute_semaphore_value = 0;
  const viewer_info &viewer_info;
};

struct graphics_frame_info {
  int frame_index;
  VkCommandBuffer command_buffer;
  VkDescriptorSet global_descriptor_set;
  frustum_info view_frustum;
};

struct physics_frame_info {
  int frame_index;
  VkCommandBuffer command_buffer;
  float dt;
};

}