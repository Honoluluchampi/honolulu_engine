// hnll
#include <graphics/texture_image.hpp>
#include <graphics/device.hpp>
#include <graphics/buffer.hpp>

// lib
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>
#include <vulkan/vulkan.h>

namespace hnll::graphics {

VkDeviceSize load_stb_image(stbi_uc* raw_data, const std::string& filepath)
{

}

u_ptr<texture_image> create(device& device, const std::string& filepath)
{ return std::make_unique<texture_image>(device, filepath); }

texture_image::texture_image(device &device, const std::string &filepath)
 : device_(device)
{
  // stb image utility
  int width, height, channels;
  stbi_uc* raw_data = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
  VkDeviceSize image_size = width * height * 4;

  if (!raw_data) { throw std::runtime_error("failed to load stb image."); }

  VkImage texture_image;
  VkDeviceMemory texture_image_memory;

  VkImageCreateInfo image_info{};
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.extent.width = static_cast<uint32_t>(width);
  image_info.extent.height = static_cast<uint32_t>(height);
  image_info.extent.depth = 1;
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
  // if you want to be able to directly access texels in the memory of the image
  // you must use VK_IMAGE_TILING_LINEAR
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL; // efficient access from the shader
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  // the texture data will be copied by staging buffer
  image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  // the image will only be used by one queue family
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  // for multisampling
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  // for memory reduction for sparse image
  image_info.flags = 0;

  if (vkCreateImage(device_.get_device(), &image_info, nullptr, &texture_image) != VK_SUCCESS)
    throw std::runtime_error("failed to create vulkan image!");
}

} // namespace hnll::graphics