#pragma once

// hnll
#include <utils/common_alias.hpp>

// std
#include <vulkan/vulkan.h>

namespace hnll::graphics {

class device;
class desc_pool;
class desc_layout;
class desc_sets;
class image_resource;

class texture_image
{
  public:
    static u_ptr<texture_image> create(device&, const std::string& filepath);

    texture_image(device& device, const std::string& filepath);
    ~texture_image();

    static void setup_desc_layout(device& device);

    // getter
    VkDescriptorImageInfo get_image_info() const;
    static VkDescriptorSetLayout get_vk_desc_layout();
    VkDescriptorSet get_vk_desc_set();

    static void reset_desc_layout();

  private:
    void create_sampler();
    void create_desc_set();

    device& device_;
    u_ptr<image_resource> image_;
    s_ptr<desc_pool>   desc_pool_;
    u_ptr<desc_sets>   desc_sets_;
    VkSampler sampler_;

    static u_ptr<desc_layout> desc_layout_;
};

} // namespace hnll::graphics