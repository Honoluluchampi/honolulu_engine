#pragma once

// hnll
#include <graphics/device.hpp>
#include <utils/utils.hpp>

// lib
#include <vulkan/vulkan.h>

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
    void create_pipeline(
      const std::string& filepath,
      const std::vector<VkDescriptorSetLayout>& desc_set_layouts
    );

    VkShaderModule shader_module_;
    VkPipelineLayout pipeline_layout_;
    VkPipeline pipeline_;

    graphics::device& device_;
};

#define CS_API template <typename Derived>
#define CS_TYPE compute_shader<Derived>

CS_API void CS_TYPE::create_pipeline(
  const std::string &filepath,
  const std::vector<VkDescriptorSetLayout> &desc_set_layouts)
{
  // create shader module
  auto raw_code = utils::read_file_for_shader(filepath);
  shader_module_ = device_.create_shader_module(raw_code);

  VkPipelineShaderStageCreateInfo shader_stage_info{};
  shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shader_stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  shader_stage_info.module = shader_module_;
  shader_stage_info.pName = "main";

  // create pipeline layout
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = desc_set_layouts.size();
  pipelineLayoutInfo.pSetLayouts = desc_set_layouts.data();

  if (vkCreatePipelineLayout(device_.get_device(), &pipelineLayoutInfo, nullptr, &pipeline_layout_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create compute pipeline layout!");
  }

  VkComputePipelineCreateInfo create_info {};
  create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  create_info.layout = pipeline_layout_;
  create_info.stage = shader_stage_info;

  if (vkCreateComputePipelines(device_.get_device(), VK_NULL_HANDLE, 1, &create_info, nullptr, &pipeline_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create compute pipeline!");
  }
}

} // namespace hnll::game