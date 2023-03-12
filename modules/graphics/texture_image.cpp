// hnll
#include <graphics/texture_image.hpp>
#include <graphics/image_resource.hpp>
#include <graphics/device.hpp>
#include <graphics/desc_set.hpp>

namespace hnll::graphics {

u_ptr<desc_layout> texture_image::desc_layout_;

u_ptr<texture_image> texture_image::create(device& device, const std::string &filepath)
{ return std::make_unique<texture_image>(device, filepath); }

texture_image::texture_image(device &device, const std::string& filepath) : device_(device)
{
  image_ = image_resource::create_from_file(device, filepath);
  create_sampler();
  create_desc_set();
}

texture_image::~texture_image()
{
  vkDestroySampler(device_.get_device(), sampler_, nullptr);
}

void texture_image::setup_desc_layout(device& device)
{
  desc_layout_ = graphics::desc_layout::builder(device)
    .add_binding(
      0,
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      VK_SHADER_STAGE_FRAGMENT_BIT)
    .build();
}

VkDescriptorImageInfo texture_image::get_image_info() const
{
  VkDescriptorImageInfo image_info{};
  image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  image_info.imageView   = image_->get_image_view();
  image_info.sampler     = sampler_;

  return image_info;
}

VkDescriptorSetLayout texture_image::get_desc_layout()
{ return desc_layout_->get_descriptor_set_layout(); }

void texture_image::create_sampler()
{
  VkSamplerCreateInfo sampler_info{};
  sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  // filtering
  sampler_info.magFilter = VK_FILTER_LINEAR;
  sampler_info.minFilter = VK_FILTER_LINEAR;
  // matters when you sample outside the image
  // repeat is common for texture tiling
  sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

  sampler_info.anisotropyEnable = VK_TRUE;
  // determine proper value for max anisotropy
  VkPhysicalDeviceProperties prop{};
  vkGetPhysicalDeviceProperties(device_.get_physical_device(), &prop);
  sampler_info.maxAnisotropy = prop.limits.maxSamplerAnisotropy;

  // when sampling beyond the image with clamp to border addressing mode
  sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

  sampler_info.unnormalizedCoordinates = VK_FALSE;

  // for percentage closer filtering on shadow maps
  sampler_info.compareEnable = VK_FALSE;
  sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;

  sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_info.mipLodBias = 0.0f;
  sampler_info.minLod = 0.0f;
  sampler_info.maxLod = 0.0f;

  if (vkCreateSampler(device_.get_device(), &sampler_info, nullptr, &sampler_) != VK_SUCCESS)
    throw std::runtime_error("failed to create sampler!");
}

void texture_image::create_desc_set()
{
  desc_pool_ = desc_pool::builder(device_)
    .set_max_sets(1)
    .add_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
    .build();

  auto image_info = get_image_info();
  desc_writer(*desc_layout_, *desc_pool_)
    .write_image(0, &image_info)
    .build(desc_set_);
}

} // namespace hnll::graphics