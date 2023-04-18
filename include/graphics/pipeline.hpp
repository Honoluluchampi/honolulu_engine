#pragma once

// hnll
#include <graphics/device.hpp>
#include <utils/common_alias.hpp>

// std
#include <vector>
#include <string>
#include <memory>

namespace hnll{

namespace graphics {

struct pipeline_config_info
{
  pipeline_config_info() = default;
  // for pipeline config info
  void create_input_assembly_info();
  // viewport and scissor
  void create_viewport_info();
  // rasterizer
  void create_rasterization_info();
  // multisampling used for anti-aliasing
  void create_multi_sample_state();
  // color blending for alpha blending
  void create_color_blend_attachment();
  void create_color_blend_state();
  void create_depth_stencil_state();
  // dynamic state
  void create_dynamic_state();

  // manually called functions
  template <typename V>
  void create_vertex_binding_descriptions()
  {
    std::vector<VkVertexInputBindingDescription> binding(1);
    // per-vertex data is packed together in one array, so the index of the
    // binding in the array is always 0
    binding[0].binding = 0;
    // number of bytes from one entry to the next
    binding[0].stride = sizeof(V);
    binding[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    binding_descriptions = binding;
  }

  void create_vertex_attribute_descriptions(
    const std::vector<VkFormat>& format_list,
    const std::vector<uint32_t>& offset_list);

  // member variables
  std::vector<VkVertexInputBindingDescription>   binding_descriptions{};
  std::vector<VkVertexInputAttributeDescription> attribute_descriptions{};
  VkPipelineInputAssemblyStateCreateInfo         input_assembly_info{};
  VkPipelineViewportStateCreateInfo              viewport_info{};
  VkPipelineRasterizationStateCreateInfo         rasterization_info{};
  VkPipelineMultisampleStateCreateInfo           multi_sample_info{};
  VkPipelineColorBlendAttachmentState            color_blend_attachment{};
  VkPipelineColorBlendStateCreateInfo            color_blend_info{};
  VkPipelineDepthStencilStateCreateInfo          depth_stencil_info{};
  std::vector<VkDynamicState>                    dynamic_state_enables;
  VkPipelineDynamicStateCreateInfo               dynamic_state_info{};
  VkPipelineLayout                               pipeline_layout = nullptr;
  VkRenderPass                                   render_pass = nullptr;
  uint32_t                                       subpass = 0;
};

class pipeline
{
  public:
    pipeline(
      device &device,
      const std::string &vert_filepath,
      const std::string &frag_filepath,
      const pipeline_config_info &config_info);
    pipeline(
      device& _device,
      const std::vector<std::string>& _shader_filepaths,
      const std::vector<VkShaderStageFlagBits>& _shader_stage_flags,
      const pipeline_config_info& _config_info);
    // for mesh shader and ray tracing pipeline
    pipeline(device &device) : device_(device){}
    ~pipeline();

    static u_ptr<pipeline> create(
      device& _device,
      const std::vector<std::string>& _shader_filepaths,
      const std::vector<VkShaderStageFlagBits>& _shader_stage_frags,
      const pipeline_config_info& _config_info);

    // uncopyable
    pipeline(const pipeline &) = delete;
    pipeline& operator=(const pipeline &) = delete;

    void bind(VkCommandBuffer command_buffer);

    static void default_pipeline_config_info(pipeline_config_info &config_info);
    static pipeline_config_info default_pipeline_config_info();
    static void enable_alpha_blending(pipeline_config_info& config_info);

    // getter
    VkPipeline get_pipeline() const { return graphics_pipeline_; }

  protected:
    device& device_;
    VkPipeline graphics_pipeline_;

  private:
    void create_graphics_pipeline(
      const std::vector<std::string>& _shader_filepaths,
      const std::vector<VkShaderStageFlagBits>& _shader_stage_flags,
      const pipeline_config_info& _config_info);

    VkPipelineShaderStageCreateInfo create_shader_stage_info(
      VkShaderModule _shader_module,
      VkShaderStageFlagBits _shader_stage_flag);

    VkPipelineVertexInputStateCreateInfo create_vertex_input_info();
    //
    std::vector<VkShaderModule> shader_modules_;
};


}} // namespace hnll::graphics