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

class image_resource
{
  public:
    static u_ptr<image_resource> create_from_file(device& device, const std::string& filepath);
    static u_ptr<image_resource> create(
      device& device,
      VkExtent3D extent,
      VkFormat image_format,
      VkImageTiling tiling,
      VkImageUsageFlags usage,
      VkMemoryPropertyFlags properties,
      bool for_ray_tracing = false);
    static u_ptr<image_resource> create_blank(device& device);

    explicit image_resource(
      device& device,
      VkExtent3D extent,
      VkFormat image_format,
      VkImageTiling tiling,
      VkImageUsageFlags usage,
      VkMemoryPropertyFlags properties,
      bool for_ray_tracing = false);
    image_resource(device& device);

    ~image_resource();

    void transition_image_layout(VkImageLayout new_layout, VkCommandBuffer manual_command = nullptr);

    void copy_buffer_to_image(
      VkBuffer buffer,
      VkExtent3D extent
    );

    // getter
    [[nodiscard]] VkImage           get_image()        const { return image_; }
    [[nodiscard]] VkImageView       get_image_view()   const { return image_view_; }
    [[nodiscard]] VkDeviceMemory    get_memory()       const { return image_memory_; }
    [[nodiscard]] VkImageLayout     get_image_layout() const { return layout_; }
    [[nodiscard]] const VkExtent2D& get_extent()       const { return extent_; }
    [[nodiscard]] VkImageSubresourceRange get_sub_resource_range() const { return sub_resource_range_; }

    // setter
    void set_image(VkImage image) { image_ = image; }
    void set_image_view(VkImageView view) { image_view_ = view; }
    void set_device_memory(VkDeviceMemory memory) { image_memory_ = memory; }

  private:
    void create_image_view(bool for_ray_tracing = false);
    void create_sampler();

    device& device_;
    VkImage image_;
    VkDeviceMemory image_memory_;
    VkImageLayout  layout_ = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageView    image_view_;
    VkFormat       image_format_;
    VkExtent2D     extent_;

    VkImageSubresourceRange sub_resource_range_ = {
      VK_IMAGE_ASPECT_COLOR_BIT,
      0, // base mip level
      1, // level count
      0, // base array layer
      1, // layer count
    };
};

} // namespace hnll::graphics