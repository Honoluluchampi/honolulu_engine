// hnll
#include <graphics/image_resource.hpp>
#include <graphics/device.hpp>
#include <graphics/buffer.hpp>

// lib
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>
#include <vulkan/vulkan.h>

namespace hnll::graphics {

VkExtent3D load_stb_image(stbi_uc* raw_data, const std::string& filepath)
{
  // stb image utility
  int width, height, channels;
  raw_data = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

  if (!raw_data) { throw std::runtime_error("failed to load stb image."); }

  return { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
}

u_ptr<image_resource> image_resource::create_texture_image(device& device, const std::string& filepath)
{
  // load raw data using stb_image
  stbi_uc* raw_data;
  auto extent = load_stb_image(raw_data, filepath);

  VkDeviceSize image_size = extent.width * extent.height * 4;

  auto staging_buffer = buffer::create(
    device,
    image_size,
    1,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    raw_data
  );

  stbi_image_free(raw_data);

  // create texture image
  VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

  auto ret = std::make_unique<image_resource>(
    device,
    extent,
    format,
    VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  // available for copy destination
  ret->transition_image_layout(
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  ret->copy_buffer_to_image(staging_buffer->get_buffer(), extent);

  // available for shader
  ret->transition_image_layout(
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
  );
}

u_ptr<image_resource> image_resource::create(
  device &device,
  VkExtent3D extent,
  VkFormat image_format,
  VkImageTiling tiling,
  VkImageUsageFlags usage,
  VkMemoryPropertyFlags properties)
{ return std::make_unique<image_resource>(device, extent, image_format, tiling, usage, properties); }

image_resource::image_resource(
  device &device,
  VkExtent3D extent,
  VkFormat image_format,
  VkImageTiling tiling,
  VkImageUsageFlags usage,
  VkMemoryPropertyFlags properties)
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

  create_image_view();
}

image_resource::~image_resource()
{
  vkDestroyImageView(device_.get_device(), image_view_, nullptr);
  vkDestroyImage(device_.get_device(), image_, nullptr);
  vkFreeMemory(device_.get_device(), image_memory_, nullptr);
}

void image_resource::transition_image_layout(
  VkImageLayout old_layout,
  VkImageLayout new_layout)
{
  auto command_buffer = device_.begin_one_shot_commands();

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  // this field could be VK_IMAGE_LAYOUT_UNDEFINED
  // if you don't care about  the existing contents
  barrier.oldLayout = old_layout;
  barrier.newLayout = new_layout;

  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

  barrier.image = image_;
  barrier.subresourceRange = sub_resource_range_;

  VkPipelineStageFlags src_stage;
  VkPipelineStageFlags dst_stage;

  if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    throw std::invalid_argument("unsupported layout transition!");
  }

  vkCmdPipelineBarrier(
    command_buffer,
    src_stage, dst_stage,
    0,
    0, nullptr,
    0, nullptr,
    1, &barrier
  );

  device_.end_one_shot_commands(command_buffer);
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

void image_resource::create_image_view()
{
  VkImageViewCreateInfo view_info{};
  view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.image = image_;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = image_format_;
  view_info.subresourceRange = sub_resource_range_;

  if (vkCreateImageView(device_.get_device(), &view_info, nullptr, &image_view_) != VK_SUCCESS)
    throw std::runtime_error("failed to create texture image view!");
}

} // namespace hnll::graphics