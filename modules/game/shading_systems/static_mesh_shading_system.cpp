// hnll
#include <game/shading_systems/static_mesh_shading_system.hpp>
#include <game/modules/graphics_engine.hpp>

namespace hnll {

namespace game {

struct mesh_push_constant
{
  mat4 model_matrix;
  mat4 normal_matrix;
};

DEFAULT_SHADING_SYSTEM_CTOR_IMPL(static_mesh_shading_system, static_mesh_comp);

void static_mesh_shading_system::setup()
{
  shading_type_ = utils::shading_type::MESH;

  pipeline_layout_ = create_pipeline_layout<mesh_push_constant>(
    static_cast<VkShaderStageFlagBits>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
    std::vector<VkDescriptorSetLayout>{
      graphics_engine_core::get_global_desc_layout(),
      graphics::texture_image::get_vk_desc_layout()
    }
  );

  auto pipeline_config_info = graphics::pipeline::default_pipeline_config_info();
  pipeline_config_info.binding_descriptions   = graphics::vertex::get_binding_descriptions();
  pipeline_config_info.attribute_descriptions = graphics::vertex::get_attribute_descriptions();

  pipeline_ = create_pipeline(
    pipeline_layout_,
    graphics_engine_core::get_default_render_pass(),
    "/modules/graphics/shaders/spv/",
    { "textured.vert.spv", "textured.frag.spv" },
    { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT },
    pipeline_config_info
  );
}

void static_mesh_shading_system::render(const utils::graphics_frame_info& frame_info)
{
  set_current_command_buffer(frame_info.command_buffer);
  bind_pipeline();

  for (auto& target : targets_) {
    auto obj = target.second;

    mesh_push_constant push{};
    const auto& tf = obj.get_transform();
    push.model_matrix  = tf.transform_mat4();
    push.normal_matrix = tf.normal_matrix();

    // desc_set
    std::vector<VkDescriptorSet> desc_sets = {
      frame_info.global_descriptor_set,
      obj.get_texture_desc_set()
    };

    bind_desc_sets(desc_sets);
    bind_push(push, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

    obj.bind(frame_info.command_buffer);
    obj.draw(frame_info.command_buffer);
  }
}

}} // namespace hnll::game