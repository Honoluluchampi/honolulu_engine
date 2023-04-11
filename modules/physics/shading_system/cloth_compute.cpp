// hnll
#include <game/modules/graphics_engine.hpp>
#include <physics/shading_system/cloth_compute.hpp>

namespace hnll::physics {

void cloth_compute_shading_system::setup()
{
  shading_type_ = utils::shading_type::MESH;

  pipeline_layout_ = create_pipeline_layout_without_push(
    static_cast<VkShaderStageFlagBits>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
    std::vector<VkDescriptorSetLayout>{
      game::graphics_engine_core::get_global_desc_layout()
    }
  );

  auto pipeline_config_info = graphics::pipeline::default_pipeline_config_info();

  pipeline_ = create_pipeline(
    pipeline_layout_,
    game::graphics_engine_core::get_default_render_pass(),
    // TODO : configure different shader directories for each shaders
    "/modules/graphics/shaders/spv",
    { "cloth_compute.vert.spv", "simple_shader.frag.spv" },
    { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT },
    pipeline_config_info
  );
}

void cloth_compute_shading_system::render(const utils::graphics_frame_info& frame_info)
{
  pipeline_->bind(frame_info.command_buffer);


}
} // namespace hnll::physics