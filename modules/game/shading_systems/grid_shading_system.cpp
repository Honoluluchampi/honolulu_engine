// hnll
#include <game/shading_systems/grid_shading_system.hpp>

namespace hnll::game {

DEFAULT_SHADING_SYSTEM_CTOR_IMPL(grid_shading_system, grid_comp);

void grid_shading_system::setup()
{
  shading_type_ = utils::shading_type::GRID;

  pipeline_layout_ = create_pipeline_layout_without_push(
    static_cast<VkShaderStageFlagBits>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
    std::vector<VkDescriptorSetLayout>{ graphics_engine_core::get_global_desc_layout() }
  );

  // delete redundant vertex attributes
  auto config_info = graphics::pipeline::default_pipeline_config_info();
  config_info.attribute_descriptions.clear();
  config_info.binding_descriptions.clear();

  pipeline_ = create_pipeline(
    pipeline_layout_,
    graphics_engine_core::get_default_render_pass(),
    "/modules/graphics/shaders/spv/",
    { "grid_shader.vert.spv", "grid_shader.frag.spv" },
    { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT },
    config_info
  );
}

void grid_shading_system::render(const utils::graphics_frame_info &frame_info)
{
  set_current_command_buffer(frame_info.command_buffer);
  bind_pipeline();
  bind_desc_sets({frame_info.global_descriptor_set});

  vkCmdDraw(frame_info.command_buffer, 6, 1, 0, 0);
}

} // namespace hnll::game