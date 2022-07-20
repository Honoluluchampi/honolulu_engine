// hnll
#include <graphics/systems/mesh_rendering_system.hpp>
#include <game/components/mesh_component.hpp>

// lib
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

//std
#include <stdexcept>
#include <array>
#include <string>

namespace hnll {
namespace graphics {

// should be compatible with a shader
struct mesh_push_constant
{
  glm::mat4 model_matrix {1.f};
  // to align data offsets with shader
  glm::mat4 normal_matrix {1.f};
};

mesh_rendering_system::mesh_rendering_system
  (device& device, VkRenderPass render_pass, VkDescriptorSetLayout global_set_layout)
  : rendering_system(device, hnll::game::render_type::SIMPLE)
{ 
  create_pipeline_layout(global_set_layout);
  create_pipeline(render_pass, "simple_shader.vert.spv", "simple_shader.frag.spv");
}

mesh_rendering_system::~mesh_rendering_system()
{}

void mesh_rendering_system::create_pipeline_layout(VkDescriptorSetLayout global_set_layout)
{
  // config push constant range
  VkPushConstantRange push_constant_range{};
  push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  // mainly for if you are going to separate ranges for the vertex and fragment shaders
  push_constant_range.offset = 0;
  push_constant_range.size = sizeof(mesh_push_constant);

  std::vector<VkDescriptorSetLayout> descriptor_set_layouts{global_set_layout};

  VkPipelineLayoutCreateInfo pipeline_layout_info{};
  pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_info.setLayoutCount = static_cast<uint32_t>(descriptor_set_layouts.size());
  pipeline_layout_info.pSetLayouts = descriptor_set_layouts.data();
  pipeline_layout_info.pushConstantRangeCount = 1;
  pipeline_layout_info.pPushConstantRanges = &push_constant_range;
  if (vkCreatePipelineLayout(device_.get_device(), &pipeline_layout_info, nullptr, &pipeline_layout_) != VK_SUCCESS)
      throw std::runtime_error("failed to create pipeline layout!");
}

void mesh_rendering_system::create_pipeline(
  VkRenderPass render_pass,
  std::string vertex_shader,
  std::string fragment_shader,
  std::string shaders_directory)
{
  assert(pipeline_layout_ != nullptr && "cannot create pipeline before pipeline layout");

  pipeline_config_info pipeline_config{};
  pipeline::default_pipeline_config_info(pipeline_config);
  pipeline_config.render_pass = render_pass;
  pipeline_config.pipeline_layout = pipeline_layout_;
  pipeline_ = std::make_unique<pipeline>(
      device_,
      shaders_directory + vertex_shader,
      shaders_directory + fragment_shader,
      pipeline_config);
}


void mesh_rendering_system::render(frame_info frame_info)
{
  pipeline_->bind(frame_info.command_buffer);

  vkCmdBindDescriptorSets(
    frame_info.command_buffer,
    VK_PIPELINE_BIND_POINT_GRAPHICS,
    pipeline_layout_,
    0, 1,
    &frame_info.global_discriptor_set,
    0, nullptr
  );

  for (auto& target : render_target_map_) {
    
    auto obj = dynamic_cast<hnll::game::mesh_component*>(target.second.get());
    if (obj->get_model_sp() == nullptr) continue;
    mesh_push_constant push{};
    // camera projection
    auto mat4 = obj->get_transform().mat4();
    push.model_matrix[0][0] = mat4(0, 0);
    push.model_matrix[0][1] = mat4(0, 1);
    push.model_matrix[0][2] = mat4(0, 2);
    push.model_matrix[0][3] = mat4(0, 3);
    push.model_matrix[1][0] = mat4(1, 0);
    push.model_matrix[1][1] = mat4(1, 1);
    push.model_matrix[1][2] = mat4(1, 2);
    push.model_matrix[1][3] = mat4(1, 3);
    push.model_matrix[2][0] = mat4(2, 0);
    push.model_matrix[2][1] = mat4(2, 1);
    push.model_matrix[2][2] = mat4(2, 2);
    push.model_matrix[2][3] = mat4(2, 3);
    push.model_matrix[3][0] = mat4(3, 0);
    push.model_matrix[3][1] = mat4(3, 1);
    push.model_matrix[3][2] = mat4(3, 2);
    push.model_matrix[3][3] = mat4(3, 3);
    // automatically converse mat3(normal_matrix) to mat4 for shader data alignment
    auto normal_mat4 = obj->get_transform().normal_matrix();
    push.normal_matrix[0][0] = normal_mat4(0, 0);
    push.normal_matrix[0][1] = normal_mat4(0, 1);
    push.normal_matrix[0][2] = normal_mat4(0, 2);
    push.normal_matrix[0][3] = normal_mat4(0, 3);
    push.normal_matrix[1][0] = normal_mat4(1, 0);
    push.normal_matrix[1][1] = normal_mat4(1, 1);
    push.normal_matrix[1][2] = normal_mat4(1, 2);
    push.normal_matrix[1][3] = normal_mat4(1, 3);
    push.normal_matrix[2][0] = normal_mat4(2, 0);
    push.normal_matrix[2][1] = normal_mat4(2, 1);
    push.normal_matrix[2][2] = normal_mat4(2, 2);
    push.normal_matrix[2][3] = normal_mat4(2, 3);
    push.normal_matrix[3][0] = normal_mat4(3, 0);
    push.normal_matrix[3][1] = normal_mat4(3, 1);
    push.normal_matrix[3][2] = normal_mat4(3, 2);
    push.normal_matrix[3][3] = normal_mat4(3, 3);

    vkCmdPushConstants(
        frame_info.command_buffer,
        pipeline_layout_, 
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
        0, 
        sizeof(mesh_push_constant), 
        &push);
    obj->get_model_sp()->bind(frame_info.command_buffer);
    obj->get_model_sp()->draw(frame_info.command_buffer);
  }
}

} // namespace graphics
} // namespace hnll