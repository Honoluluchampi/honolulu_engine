// hnll
#include "include/fdtd2_shading_system.hpp"
#include "include/fdtd2_field.hpp"
#include <game/modules/graphics_engine.hpp>

namespace hnll {

fdtd2_field* fdtd2_shading_system::target_ = nullptr;
uint32_t fdtd2_shading_system::target_id_ = -1;

struct fdtd2_frag_push {
  float width;
  float height;
  int x_grid;
  int y_grid;
};

DEFAULT_SHADING_SYSTEM_CTOR_IMPL(fdtd2_shading_system, game::dummy_renderable_comp<utils::shading_type::MESH>);
fdtd2_shading_system::~fdtd2_shading_system()
{}

void fdtd2_shading_system::setup()
{
  shading_type_ = utils::shading_type::MESH;
  desc_layout_ = graphics::desc_layout::create_from_bindings(*device_, fdtd2_field::field_bindings);
  auto vk_layout = desc_layout_->get_descriptor_set_layout();

  pipeline_layout_ = create_pipeline_layout<fdtd2_frag_push>(
    static_cast<VkShaderStageFlagBits>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
    std::vector<VkDescriptorSetLayout>{
      vk_layout
    }
  );

  auto pipeline_config_info = graphics::pipeline::default_pipeline_config_info();
  pipeline_ = create_pipeline(
    pipeline_layout_,
    game::graphics_engine_core::get_default_render_pass(),
    "/examples/fdtd_compute/fdtd_2d/shaders/spv/",
    { "fdtd2_field.vert.spv", "fdtd2_field.frag.spv" },
    { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT },
    pipeline_config_info
  );
}

void fdtd2_shading_system::render(const utils::graphics_frame_info &frame_info)
{
  if (target_ != nullptr) {
    auto command = frame_info.command_buffer;
    pipeline_->bind(command);

    auto window_size = game::gui_engine::get_viewport_size();
    fdtd2_frag_push push;
    push.width = window_size.x;
    push.height = window_size.y;
    push.x_grid = target_->get_x_grid();
    push.y_grid = target_->get_y_grid();
    vkCmdPushConstants(
      command,
      pipeline_layout_,
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      0,
      sizeof(fdtd2_frag_push),
      &push);

    auto desc_sets = target_->get_frame_desc_sets(0)[2];
    vkCmdBindDescriptorSets(
      command,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline_layout_,
      0,
      1,
      &desc_sets,
      0,
      nullptr
    );

    vkCmdDraw(command, 6, 1, 0, 0);

    target_->update_frame();
  }
}

void fdtd2_shading_system::set_target(fdtd2_field* target)
{ target_ = target; }

void fdtd2_shading_system::remove_target(uint32_t target_id)
{ if (target_id_ == target_id) target_ = nullptr; }

} // namespace hnll