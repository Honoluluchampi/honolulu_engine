#pragma once

// hnll
#include <utils/common_alias.hpp>

// std
#include <string>

// lib
#include <vulkan/vulkan.h>

namespace hnll::graphics {

class device;
class buffer;

class image
{
  public:
    u_ptr<image> create_texture_image(device& device, const std::string& filepath);
    u_ptr<image> create(
      device& device,
      VkExtent3D extent,
      VkFormat image_format,
      VkImageTiling tiling,
      VkImageUsageFlags usage,
      VkMemoryPropertyFlags properties);

    explicit image(
      device& device,
      VkExtent3D extent,
      VkFormat image_format,
      VkImageTiling tiling,
      VkImageUsageFlags usage,
      VkMemoryPropertyFlags properties);

    ~image();

  private:
    void transition_image_layout(
      VkFormat format,
      VkImageLayout old_layout,
      VkImageLayout new_layout
    );

    void copy_buffer_to_image(
      VkBuffer buffer,
      VkExtent3D extent
    );

    device& device_;
    VkImage image_;
    VkDeviceMemory image_memory_;
};

} // namespace hnll::graphics