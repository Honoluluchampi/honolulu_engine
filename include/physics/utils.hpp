#pragma once

// lib
#include <vulkan/vulkan.h>

namespace hnll::physics {

struct frame_info
{
  VkCommandBuffer command_buffer;
  float dt;
};

} // namespace hnll::physics