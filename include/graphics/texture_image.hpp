#pragma once

// hnll
#include <utils/common_alias.hpp>

// std
#include <vulkan/vulkan.h>

namespace hnll::graphics {

class device;
class desc_pool;
class desc_layout;
class desc_set;
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
    static VkDescriptorSetLayout get_desc_layout();

  private:
    void create_sampler();
    void create_desc_set();

    device& device_;
    u_ptr<image_resource> image_;
    u_ptr<desc_pool>   desc_pool_;
    VkDescriptorSet    desc_set_;
    VkSampler sampler_;

    static u_ptr<desc_layout> desc_layout_;
};

} // namespace hnll::graphics