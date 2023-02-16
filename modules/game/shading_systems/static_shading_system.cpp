// hnll
#include <game/modules/graphics_engine.hpp>
#include <game/renderable_component.hpp>

namespace hnll {

namespace game {

using static_mesh_comp = renderable_comp<utils::shading_type::MESH>;
using static_mesh_shading_system = shading_system<static_mesh_comp>;

struct mesh_push_constant
{
  mat4 model_matrix;
  mat4 normal_matrix;
};

template <>
u_ptr<static_mesh_shading_system> static_mesh_shading_system::create(graphics::device& device)
{ return std::make_unique<static_mesh_shading_system>(device); }

template <>
static_mesh_shading_system::shading_system(graphics::device &device)
  : device_(device), shading_type_(utils::shading_type::MESH)
{
  pipeline_layout_ = create_pipeline_layout<mesh_push_constant>(
    static_cast<VkShaderStageFlagBits>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
    std::vector<VkDescriptorSetLayout>{ graphics_engine_core::get_global_desc_layout() }
  );

  auto pipeline_config_info = graphics::pipeline::default_pipeline_config_info();
  pipeline_config_info.binding_descriptions   = graphics::vertex::get_binding_descriptions();
  pipeline_config_info.attribute_descriptions = graphics::vertex::get_attribute_descriptions();

  pipeline_ = create_pipeline(
    pipeline_layout_,
    graphics_engine_core::get_default_render_pass(),
    "/modules/graphics/shaders/spv/",
    { "simple_shader.vert.spv", "simple_shader.frag.spv" },
    { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT },
    pipeline_config_info
  );
}

template <>
void static_mesh_shading_system::render(const utils::frame_info& frame_info)
{
  pipeline_->bind(frame_info.command_buffer);

  vkCmdBindDescriptorSets(
    frame_info.command_buffer,
    VK_PIPELINE_BIND_POINT_GRAPHICS,
    pipeline_layout_,
    0,
    1,
    &frame_info.global_descriptor_set,
    0,
    nullptr
  );

  for (auto& target : targets_) {
    auto obj = target.second;

    mesh_push_constant push{};
    const auto& tf = obj.get_transform();
    push.model_matrix  = tf.transform_mat4();
    push.normal_matrix = tf.normal_matrix();

    vkCmdPushConstants(
      frame_info.command_buffer,
      pipeline_layout_,
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      0,
      sizeof(mesh_push_constant),
      &push
    );

    obj.bind(frame_info.command_buffer);
    obj.draw(frame_info.command_buffer);
  }
}

}} // namespace hnll::game