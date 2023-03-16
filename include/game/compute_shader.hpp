#pragma once

#include <vulkan/vulkan.h>
#include <graphics/device.hpp>

namespace hnll::game {

template <typename Derived>
class compute_shader
{
  public:
    compute_shader(graphics::device& device) : device_(device)
    { static_cast<Derived>(this)->setup(); }
  protected:
    // write this function for each compute shader
    void setup();
    void create_pipeline_layout(const std::vector<VkDescriptorSetLayout>& desc_set_layouts);
    void create_pipeline();

    VkPipeline pipeline_;
    VkPipelineLayout pipeline_layout_;

    graphics::device& device_;
};

#define CS_API template <typename Derived>
#define CS_TYPE compute_shader<Derived>

CS_API void CS_TYPE::create_pipeline_layout(const std::vector<VkDescriptorSetLayout>& desc_set_layouts)
{
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = desc_set_layouts.size();
  pipelineLayoutInfo.pSetLayouts = desc_set_layouts.data();

  if (vkCreatePipelineLayout(device_.get_device(), &pipelineLayoutInfo, nullptr, &pipeline_layout_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create compute pipeline layout!");
  }
}

CS_API void CS_TYPE::create_pipeline()
{
  // create shader module
  auto shader_module = device_.create_shader_module()

  VkComputePipelineCreateInfo create_info {};
  create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  create_info.layout = pipeline_layout_;
  create_info.stage = computeShaderStageInfo;

  if (vkCreateComputePipelines(device_.get_device(), VK_NULL_HANDLE, 1, &create_info, nullptr, &pipeline_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create compute pipeline!");
  }
}

} // namespace hnll::game