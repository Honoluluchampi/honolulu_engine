#pragma once

// hnll
#include <graphics/device.hpp>
#include <utils/utils.hpp>
#include <utils/frame_info.hpp>

// lib
#include <vulkan/vulkan.h>

#define DEFINE_COMPUTE_SHADER(new_shader) class new_shader : public game::compute_shader<new_shader>
#define DEFAULT_COMPUTE_SHADER_CTOR(new_shader) explicit new_shader(graphics::device& device)
#define DEFAULT_COMPUTE_SHADER_CTOR_IMPL(shader) shader::shader(graphics::device& device) : game::compute_shader<shader>(device) { }

namespace hnll::game {

template <typename Derived>
class compute_shader
{
  public:
    explicit compute_shader(graphics::device& device) : device_(device) { static_cast<Derived*>(this)->setup(); }
    static u_ptr<Derived> create(graphics::device& device)
    { return std::make_unique<Derived>(device); }

    ~compute_shader()
    {
      vkDestroyPipelineLayout(device_.get_device(), pipeline_layout_, nullptr);
      vkDestroyPipeline(device_.get_device(), pipeline_, nullptr);
      vkDestroyShaderModule(device_.get_device(), shader_module_, nullptr);
    }

    // for physics simulation
    void render(const utils::compute_frame_info& info);

  protected:
    // write this function for each compute shader
    void setup();

    void create_pipeline(
      const std::string& filepath,
      const std::vector<VkDescriptorSetLayout>& desc_set_layouts
    );

    // compute command dispatching
    inline void bind_pipeline(VkCommandBuffer command)
    { vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_); }

    inline void bind_desc_sets(VkCommandBuffer command, const std::vector<VkDescriptorSet>& desc_sets)
    { vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout_, 0, desc_sets.size(), desc_sets.data(), 0, 0); }

    inline void dispatch_command(VkCommandBuffer command, int x, int y, int z)
    { vkCmdDispatch(command, x, y, z); }

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