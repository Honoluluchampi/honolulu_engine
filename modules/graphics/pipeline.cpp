// hnll
#include <graphics/pipeline.hpp>
#include <utils/utils.hpp>

// std
#include <iostream>
#include <stdexcept>

namespace hnll::graphics {

// what kind of geometry will be drawn from the vertices (topoloby) and
// if primitive restart should be enabled
void pipeline_config_info::create_input_assembly_info()
{
  input_assembly_info.sType =
    VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly_info.primitiveRestartEnable = VK_FALSE;
}

void pipeline_config_info::create_viewport_info()
{

  viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  // by enabling a GPU feature in logical device creation,
  // its possible to use multiple viewports
  viewport_info.viewportCount = 1;
  viewport_info.pViewports = nullptr;
  viewport_info.scissorCount = 1;
  viewport_info.pScissors = nullptr;
}

void pipeline_config_info::create_rasterization_info()
{
  rasterization_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  // using this requires enabling a GPU feature
  rasterization_info.depthClampEnable = VK_FALSE;
  // if rasterizationInfo_mDiscardEnable is set to VK_TRUE, then geometry never passes
  // through the rasterization_info stage, basically disables any output to the frame_buffer
  rasterization_info.rasterizerDiscardEnable = VK_FALSE;
  // how fragments are generated for geometry
  // using any mode other than fill requires GPU feature
  rasterization_info.polygonMode = VK_POLYGON_MODE_FILL;
  rasterization_info.lineWidth = 1.0f;
  // rasterization_info.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterization_info.cullMode = VK_CULL_MODE_NONE;
  rasterization_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
  // consider this when shadow mapping is necessary
  rasterization_info.depthBiasEnable = VK_FALSE;
  rasterization_info.depthBiasConstantFactor = 0.0f;
  rasterization_info.depthBiasClamp = 0.0f;
  rasterization_info.depthBiasSlopeFactor = 0.0f;
}

// used for anti-aliasing
void pipeline_config_info::create_multi_sample_state()
{
  multi_sample_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multi_sample_info.sampleShadingEnable = VK_FALSE;
  multi_sample_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multi_sample_info.minSampleShading = 1.0f;
  multi_sample_info.pSampleMask = nullptr;
  multi_sample_info.alphaToCoverageEnable = VK_FALSE;
  multi_sample_info.alphaToOneEnable = VK_FALSE;
}

// color blending for alpha blending
void pipeline_config_info::create_color_blend_attachment()
{
  // per framebuffer struct
  // in contrast, VkPipelineColorBlendStateCreateInfo is global color blending settings
  color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                          VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color_blend_attachment.blendEnable = VK_FALSE;
  color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
  color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
  color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
  color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; //Optional
  color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; //Optional
  color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

  color_blend_attachment.blendEnable = VK_TRUE;
  color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  color_blend_attachment.dstColorBlendFactor =VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
  color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

void pipeline_config_info::create_color_blend_state()
{
  color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blend_info.logicOpEnable = VK_FALSE;
  color_blend_info.logicOp = VK_LOGIC_OP_COPY; // Optional
  color_blend_info.attachmentCount = 1;
  color_blend_info.pAttachments = &color_blend_attachment;
  color_blend_info.blendConstants[0] = 0.0f; // Optional
  color_blend_info.blendConstants[1] = 0.0f; // Optional
  color_blend_info.blendConstants[2] = 0.0f; // Optional
  color_blend_info.blendConstants[3] = 0.0f; // Optional
}

void pipeline_config_info::create_depth_stencil_state()
{
  depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depth_stencil_info.depthTestEnable = VK_TRUE;
  depth_stencil_info.depthWriteEnable = VK_TRUE;
  depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;
  depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
  depth_stencil_info.minDepthBounds = 0.0f;  // Optional
  depth_stencil_info.maxDepthBounds = 1.0f;  // Optional
  depth_stencil_info.stencilTestEnable = VK_FALSE;
  depth_stencil_info.front = {};  // Optional
  depth_stencil_info.back = {};   // Optional
}

// a limited amount of the state can be actually be changed without recreating the pipeline
void pipeline_config_info::create_dynamic_state()
{
  dynamic_state_enables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state_info.dynamicStateCount = static_cast<uint32_t>(dynamic_state_enables.size());
  dynamic_state_info.pDynamicStates = dynamic_state_enables.data();
  dynamic_state_info.flags = 0;
}


pipeline::pipeline(
  device &device,
  const std::string &vertex_file_path,
  const std::string &fragment_file_path,
  const pipeline_config_info &config_info) : device_(device)
{
  std::vector<std::string> shader_file_paths { vertex_file_path, fragment_file_path };
  std::vector<VkShaderStageFlagBits> shader_stage_flags { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };
  create_graphics_pipeline(shader_file_paths, shader_stage_flags, config_info); }

pipeline::pipeline(
  hnll::graphics::device &_device,
  const std::vector<std::string>& _shader_filepaths,
  const std::vector<VkShaderStageFlagBits>& _shader_stage_flags,
  const hnll::graphics::pipeline_config_info& _config_info) : device_(_device)
{ create_graphics_pipeline(_shader_filepaths, _shader_stage_flags, _config_info); }

pipeline::~pipeline()
{
  auto _device = device_.get_device();
  for (auto& module : shader_modules_) {
    vkDestroyShaderModule(_device, module, nullptr);
  }
  vkDestroyPipeline(_device, graphics_pipeline_, nullptr);
}

u_ptr<pipeline> pipeline::create(
  device& _device,
  const std::vector<std::string>& _shader_filepaths,
  const std::vector<VkShaderStageFlagBits>& _shader_stage_flags,
  const pipeline_config_info& _config_info)
{
  return std::make_unique<pipeline>(_device, _shader_filepaths, _shader_stage_flags, _config_info);
}

void pipeline::create_graphics_pipeline(
  const std::vector<std::string>& _shader_filepaths,
  const std::vector<VkShaderStageFlagBits>& _shader_stage_flags,
  const pipeline_config_info& _config_info)
{
  assert(_shader_stage_flags.size() == _shader_filepaths.size());

  // if there is a vertex shader, create vertex input state
  bool has_vertex_shader = false;

  uint32_t shader_stage_count = _shader_filepaths.size();

  // create shader module
  shader_modules_.resize(shader_stage_count);
  for (int i = 0; i < shader_stage_count; i++) {
    auto raw_code = utils::read_file_for_shader(_shader_filepaths[i]);
    shader_modules_[i] = device_.create_shader_module(raw_code);

    if (_shader_stage_flags[i] == VK_SHADER_STAGE_VERTEX_BIT)
      has_vertex_shader = true;
  }

  // create shader_stage_create_info
  VkPipelineShaderStageCreateInfo shader_stages[shader_stage_count];
  for (int i = 0; i < shader_stage_count; i++) {
    shader_stages[i] = create_shader_stage_info(
      shader_modules_[i],
      _shader_stage_flags[i]
    );
  }

  VkPipelineVertexInputStateCreateInfo vertex_input_info;
  if (has_vertex_shader) {
    vertex_input_info = create_vertex_input_info();

    // accept vertex data
    auto &binding_descriptions = _config_info.binding_descriptions;
    auto &attribute_descriptions = _config_info.attribute_descriptions;
    vertex_input_info.vertexBindingDescriptionCount = static_cast<uint32_t>(binding_descriptions.size());
    vertex_input_info.pVertexBindingDescriptions = binding_descriptions.data(); //optional
    vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
    vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data(); //optional
  }

  VkGraphicsPipelineCreateInfo pipeline_info{};
  pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  // programmable stage count
  pipeline_info.stageCount          = shader_stage_count;
  pipeline_info.pStages             = shader_stages;
  if (has_vertex_shader)
    pipeline_info.pVertexInputState = &vertex_input_info;
  else
    pipeline_info.pVertexInputState = nullptr;
  pipeline_info.pInputAssemblyState = &_config_info.input_assembly_info;
  pipeline_info.pViewportState      = &_config_info.viewport_info;
  pipeline_info.pRasterizationState = &_config_info.rasterization_info;
  pipeline_info.pMultisampleState   = &_config_info.multi_sample_info;
  pipeline_info.pColorBlendState    = &_config_info.color_blend_info;
  pipeline_info.pDepthStencilState  = &_config_info.depth_stencil_info;
  pipeline_info.pDynamicState       = &_config_info.dynamic_state_info;

  pipeline_info.layout     = _config_info.pipeline_layout;
  pipeline_info.renderPass = _config_info.render_pass;
  pipeline_info.subpass    = _config_info.subpass;

  pipeline_info.basePipelineIndex  = -1;
  pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

  // its possible to create multiple VkPipeline objects in a single call
  // second parameter means cache objects enables significantly faster creation
  if (vkCreateGraphicsPipelines(device_.get_device(), VK_NULL_HANDLE, 1,
                                &pipeline_info, nullptr, &graphics_pipeline_) != VK_SUCCESS)
    throw std::runtime_error("failed to create graphics pipeline!");
}

VkPipelineShaderStageCreateInfo pipeline::create_shader_stage_info(
  VkShaderModule _shader_module,
  VkShaderStageFlagBits _shader_stage_frag)
{
  VkPipelineShaderStageCreateInfo fragment_shader_stage_info{};
  fragment_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragment_shader_stage_info.stage = _shader_stage_frag;
  fragment_shader_stage_info.module = _shader_module;
  // the function to invoke
  fragment_shader_stage_info.pName = "main";
  return fragment_shader_stage_info;
}

VkPipelineVertexInputStateCreateInfo pipeline::create_vertex_input_info()
{
  VkPipelineVertexInputStateCreateInfo vertex_input_info{};
  vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  return vertex_input_info;
}

void pipeline::bind(VkCommandBuffer command_buffer)
{
  // basic drawing commands
  // bind the graphics pipeline
  // the second parameter specifies if the pipeline object is a graphics or compute pipeline or ray tracer
  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_);
}

void pipeline::default_pipeline_config_info(pipeline_config_info &config_info)
{
  config_info.create_input_assembly_info();
  config_info.create_viewport_info();
  config_info.create_rasterization_info();
  config_info.create_multi_sample_state();
  config_info.create_color_blend_attachment();
  config_info.create_color_blend_state();
  config_info.create_depth_stencil_state();
  config_info.create_dynamic_state();
}

pipeline_config_info pipeline::default_pipeline_config_info()
{
  pipeline_config_info ci;
  default_pipeline_config_info(ci);
  return ci;
}

void pipeline::enable_alpha_blending(pipeline_config_info &config_info)
{
  config_info.color_blend_attachment.blendEnable = VK_TRUE;

  config_info.color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                                      VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  config_info.color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  config_info.color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  config_info.color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
  config_info.color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; //Optional
  config_info.color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; //Optional
  config_info.color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
}
} // namespace hnll::graphics