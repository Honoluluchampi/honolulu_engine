// hnll
#include <graphics/systems/point_light.hpp>
#include <game/components/point_light_component.hpp>

//std
#include <stdexcept>
#include <array>

namespace hnll {
namespace graphics {

struct point_light_push_constant
{
  Eigen::Vector4f position = Eigen::Vector4f::Identity();
  Eigen::Vector4f color = Eigen::Vector4f::Identity();
  float radius;
};

point_light_rendering_system::point_light_rendering_system
  (device& device, VkRenderPass render_pass, VkDescriptorSetLayout global_set_layout)
  : rendering_system(device, hnll::game::render_type::POINT_LIGHT)
{
  create_pipeline_layout(global_set_layout);
  create_pipeline(render_pass, "point_light.vert.spv", "point_light.frag.spv");
}

point_light_rendering_system::~point_light_rendering_system()
{ }

void point_light_rendering_system::create_pipeline_layout(VkDescriptorSetLayout global_set_layout)
{
  // config push constant range
  VkPushConstantRange push_constant_range{};
  push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  // mainly for if you are going to separate ranges for the vertex and fragment shaders
  push_constant_range.offset = 0;
  push_constant_range.size = sizeof(point_light_push_constant);

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

void point_light_rendering_system::create_pipeline(
  VkRenderPass render_pass,
  std::string vertex_shader,
  std::string fragment_shader,
  std::string shaders_directory)
{
  assert(pipeline_layout_ != nullptr && "cannot create pipeline before pipeline layout");

  pipeline_config_info pipeline_config{};
  pipeline::default_pipeline_config_info(pipeline_config);
  // for special config 
  pipeline_config.attribute_descriptions.clear();
  pipeline_config.binding_descriptions.clear();

  pipeline_config.render_pass = render_pass;
  pipeline_config.pipeline_layout = pipeline_layout_;
  pipeline_ = std::make_unique<pipeline>(
      device_,
      shaders_directory + vertex_shader,
      shaders_directory + fragment_shader,
      pipeline_config);
}

void point_light_rendering_system::render(frame_info frame_info)
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

  // copy the push constants
  for (auto& kv : render_target_map_) {
    auto light_comp = dynamic_cast<hnll::game::point_light_component*>(kv.second.get());

    point_light_push_constant push{};
    push.position << light_comp->get_transform().translation.cast<float>(), 1.f;
    push.color << light_comp->get_color().cast<float>(), light_comp->get_light_info().light_intensity;
    push.radius = static_cast<float>(light_comp->get_transform().scale.x());

    vkCmdPushConstants(
      frame_info.command_buffer,
      pipeline_layout_,
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      0, 
      sizeof(point_light_push_constant),
      &push
    );
    vkCmdDraw(frame_info.command_buffer, 6, 1, 0, 0);
  }
}

} // namespace graphics
} // namespace hnll