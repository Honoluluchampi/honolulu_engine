// hnll
#include <graphics/compute_pipeline.hpp>
#include <graphics/device.hpp>
#include <utils/utils.hpp>

namespace hnll::graphics {

u_ptr<compute_pipeline> compute_pipeline::create(
  device& device,
  const std::vector<VkDescriptorSetLayout>& desc_layouts,
  const std::string& filepath, // path of compute shader (spv)
  uint32_t push_size
  )
{ return std::make_unique<compute_pipeline>(device, desc_layouts, filepath, push_size);}

compute_pipeline::compute_pipeline(
  device& device,
  const std::vector<VkDescriptorSetLayout> &desc_layouts,
  const std::string &filepath,
  uint32_t push_size
  ) : device_(device)
{
  // configure push constant
  VkPushConstantRange push{};
  push.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  push.offset = 0;
  push.size = push_size;

  // create pipeline layout
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = desc_layouts.size();
  pipelineLayoutInfo.pSetLayouts = desc_layouts.data();
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &push;

  if (vkCreatePipelineLayout(device_.get_device(), &pipelineLayoutInfo, nullptr, &layout_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create compute pipeline layout!");
  }

  // create shader module
  auto raw_code = utils::read_file_for_shader(filepath);
  shader_module_ = device_.create_shader_module(raw_code);

  VkPipelineShaderStageCreateInfo shader_stage_info{};
  shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shader_stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  shader_stage_info.module = shader_module_;
  shader_stage_info.pName = "main";

  VkComputePipelineCreateInfo create_info {};
  create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  create_info.layout = layout_;
  create_info.stage = shader_stage_info;

  if (vkCreateComputePipelines(device_.get_device(), VK_NULL_HANDLE, 1, &create_info, nullptr, &pipeline_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create compute pipeline!");
  }
}

compute_pipeline::~compute_pipeline()
{
  vkDestroyPipeline(device_.get_device(), pipeline_, nullptr);
  vkDestroyShaderModule(device_.get_device(), shader_module_, nullptr);
  vkDestroyPipelineLayout(device_.get_device(), layout_, nullptr);
}

} // namespace hnll::graphics