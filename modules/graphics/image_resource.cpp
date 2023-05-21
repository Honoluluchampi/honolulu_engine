// hnll
#include <graphics/image_resource.hpp>
#include <graphics/device.hpp>
#include <graphics/buffer.hpp>

// lib
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>
#include <vulkan/vulkan.h>

namespace hnll::graphics {

u_ptr<image_resource> image_resource::create_from_file(device& device, const std::string& filepath)
{
  // load raw data using stb_image
  int width, height, channels;
  stbi_uc* pixels = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

  VkDeviceSize image_size = width * height * 4;

  auto staging_buffer = buffer::create(
    device,
    image_size,
    1,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    pixels
  );

  VkExtent3D extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
  free(pixels);

  // create texture image
  VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

  auto ret = image_resource::create(
    device,
    extent,
    format,
    VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  // available for copy destination
  ret->transition_image_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  ret->copy_buffer_to_image(staging_buffer->get_buffer(), extent);

  // available for shader
  ret->transition_image_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
  );

  return ret;
}

u_ptr<image_resource> image_resource::create(
  device &device,
  VkExtent3D extent,
  VkFormat image_format,
  VkImageTiling tiling,
  VkImageUsageFlags usage,
  VkMemoryPropertyFlags properties,
  bool for_ray_tracing)
{ return std::make_unique<image_resource>(device, extent, image_format, tiling, usage, properties, for_ray_tracing); }

u_ptr<image_resource> image_resource::create_blank(device& device)
{ return std::make_unique<image_resource>(device); }

image_resource::image_resource(
  device &device,
  VkExtent3D extent,
  VkFormat image_format,
  VkImageTiling tiling,
  VkImageUsageFlags usage,
  VkMemoryPropertyFlags properties,
  bool for_ray_tracing)
 : device_(device), image_format_(image_format)
{
  extent_ = { extent.width, extent.height };
  // create image
  VkImageCreateInfo image_info{};
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.extent = extent;
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.format = image_format_;
  // if you want to be able to directly access texels in the memory of the image
  // you must use VK_IMAGE_TILING_LINEAR
  image_info.tiling = tiling; // efficient access from the shader
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  // the texture data will be copied by staging buffer
  image_info.usage = usage;
  // the image will only be used by one queue family
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  // for multisampling
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  // for memory reduction for sparse image
  image_info.flags = 0;

  if (vkCreateImage(device_.get_device(), &image_info, nullptr, &image_) != VK_SUCCESS)
    throw std::runtime_error("failed to create vulkan image!");

  // allocate image memory
  VkMemoryRequirements mem_requirements;
  vkGetImageMemoryRequirements(device_.get_device(), image_, &mem_requirements);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex = device_.find_memory_type(
    mem_requirements.memoryTypeBits,
    properties
  );

  if (vkAllocateMemory(device_.get_device(), &alloc_info, nullptr, &image_memory_) != VK_SUCCESS)
    throw std::runtime_error("failed to allocate image memory!");

  vkBindImageMemory(device_.get_device(), image_, image_memory_, 0);

  create_image_view(for_ray_tracing);
}

image_resource::image_resource(device &device) : device_(device) {}

image_resource::~image_resource()
{
  vkDestroyImageView(device_.get_device(), image_view_, nullptr);
  vkDestroyImage(device_.get_device(), image_, nullptr);
  vkFreeMemory(device_.get_device(), image_memory_, nullptr);
}

void image_resource::transition_image_layout(VkImageLayout new_layout, VkCommandBuffer manual_command)
{
  VkCommandBuffer command_buffer;
  // if manual command buffer is not assigned
  if (manual_command == nullptr) {
    command_buffer = device_.begin_one_shot_commands();
  }
  else {
    command_buffer = manual_command;
  }

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  // this field could be VK_IMAGE_LAYOUT_UNDEFINED
  // if you don't care about  the existing contents
  barrier.oldLayout = layout_;
  barrier.newLayout = new_layout;

  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

  barrier.image = image_;
  barrier.subresourceRange = sub_resource_range_;

  VkPipelineStageFlags src_stage = 0;
  VkPipelineStageFlags dst_stage = 0;

  // current layout
  if (layout_ == VK_IMAGE_LAYOUT_UNDEFINED) {
    barrier.srcAccessMask = 0;
    src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  }
  else if (layout_ == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }

  // new layout
  if (new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if (new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }

//  else {
//    throw std::invalid_argument("unsupported layout transition!");
//  }

  vkCmdPipelineBarrier(
    command_buffer,
    src_stage, dst_stage,
    0,
    0, nullptr,
    0, nullptr,
    1, &barrier
  );

  if (manual_command == nullptr) {
    device_.end_one_shot_commands(command_buffer);
  }
  layout_ = new_layout;
}

void image_resource::copy_buffer_to_image(VkBuffer buffer, VkExtent3D extent)
{
  auto command_buffer = device_.begin_one_shot_commands();

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = { 0, 0, 0 };
  region.imageExtent = extent;

  vkCmdCopyBufferToImage(
    command_buffer,
    buffer,
    image_,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &region
  );

  device_.end_one_shot_commands(command_buffer);
}

void image_resource::create_image_view(bool for_ray_tracing)
{
  VkImageViewCreateInfo view_info{};
  view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.image = image_;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = image_format_;
  view_info.subresourceRange = sub_resource_range_;

  if (for_ray_tracing) {
    view_info.components = VkComponentMapping{
      VK_COMPONENT_SWIZZLE_R,
      VK_COMPONENT_SWIZZLE_G,
      VK_COMPONENT_SWIZZLE_B,
      VK_COMPONENT_SWIZZLE_A,
    };
  }

  if (vkCreateImageView(device_.get_device(), &view_info, nullptr, &image_view_) != VK_SUCCESS)
    throw std::runtime_error("failed to create texture image view!");
}



} // namespace hnll::graphics