// hnll
#include "include/fdtd_cylindrical_shading_system.hpp"
#include "include/fdtd_cylindrical_field.hpp"
#include <game/modules/graphics_engine.hpp>

namespace hnll {

fdtd_cylindrical_field* fdtd_cylindrical_shading_system::target_ = nullptr;
uint32_t fdtd_cylindrical_shading_system::target_id_ = -1;

DEFAULT_SHADING_SYSTEM_CTOR_IMPL(fdtd_cylindrical_shading_system, game::dummy_renderable_comp<utils::shading_type::MESH>);
fdtd_cylindrical_shading_system::~fdtd_cylindrical_shading_system()
{}

#include "common/fdtd_cylindrical.h"

void fdtd_cylindrical_shading_system::setup()
{
  shading_type_ = utils::shading_type::MESH;
  desc_layout_ = graphics::desc_layout::create_from_bindings(*device_, fdtd_cylindrical_field::field_bindings);
  auto vk_layout = desc_layout_->get_descriptor_set_layout();

  pipeline_layout_ = create_pipeline_layout<fdtd_cylindrical_frag_push>(
    static_cast<VkShaderStageFlagBits>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
    std::vector<VkDescriptorSetLayout>{
      vk_layout
    }
  );

  auto pipeline_config_info = graphics::pipeline::default_pipeline_config_info();
  pipeline_ = create_pipeline(
    pipeline_layout_,
    game::graphics_engine_core::get_default_render_pass(),
    "/examples/fdtd_compute/fdtd_cylindrical/shaders/spv/",
    { "fdtd_cylindrical_field.vert.spv", "fdtd_cylindrical_field.frag.spv" },
    { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT },
    pipeline_config_info
  );
}

void fdtd_cylindrical_shading_system::render(const utils::graphics_frame_info &frame_info)
{
  if (target_ != nullptr) {
    auto command = frame_info.command_buffer;
    pipeline_->bind(command);

    auto window_size = game::gui_engine::get_viewport_size();
    fdtd_cylindrical_frag_push push {
      .z_pixel_count = int(window_size.x),
      .r_pixel_count = int(window_size.y),
      .z_grid_count = target_->get_z_grid_count(),
      .r_grid_count = target_->get_r_grid_count()
    };

    vkCmdPushConstants(
      command,
      pipeline_layout_,
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      0,
      sizeof(fdtd_cylindrical_frag_push),
      &push);

    auto desc_sets = target_->get_frame_desc_sets()[2];
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

void fdtd_cylindrical_shading_system::set_target(fdtd_cylindrical_field* target)
{ target_ = target; }

void fdtd_cylindrical_shading_system::remove_target(uint32_t target_id)
{ if (target_id_ == target_id) target_ = nullptr; }

} // namespace hnll