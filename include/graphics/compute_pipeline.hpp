#pragma once

// hnll
#include <utils/common_alias.hpp>

// lib
#include <vulkan/vulkan.h>

namespace hnll::graphics {

class device;

class compute_pipeline
{
  public:
    static u_ptr<compute_pipeline> create(
      device& device,
      const std::vector<VkDescriptorSetLayout>& desc_layouts,
      const std::string& filepath, // path of compute shader (spv)
      uint32_t push_size
    );
    compute_pipeline(
      device& device,
      const std::vector<VkDescriptorSetLayout>& desc_layouts,
      const std::string& filepath, // path of compute shader (spv)
      uint32_t push_size
    );
    ~compute_pipeline();

    // getter
    VkPipeline       get_vk_pipeline()      const { return pipeline_; }
    VkPipelineLayout get_vk_layout()        const { return layout_; }
    VkShaderModule   get_vk_shader_module() const { return shader_module_; }

  private:
    device& device_;
    VkPipeline pipeline_;
    VkPipelineLayout layout_;
    // compute pipeline can have only 1 shader module
    VkShaderModule shader_module_;
};

} // namespace hnll::graphics