// hnll
#include <graphics/desc_set.hpp>
#include <physics/fdtd2_field.hpp>
#include <physics/compute_shader/fdtd2_compute_shader.hpp>
#include <utils/singleton.hpp>

namespace hnll::physics {

// shared with shaders
#include "../common/fdtd2_config.h"

// static member
fdtd2_field* fdtd2_compute_shader::target_ = nullptr;

fdtd2_compute_shader::fdtd2_compute_shader(graphics::device &device) : game::compute_shader<fdtd2_compute_shader>(device)
{
  desc_layout_ = graphics::desc_layout::create_from_bindings(device_, fdtd2_field::field_bindings);
  auto vk_layout = desc_layout_->get_descriptor_set_layout();

  pipeline_ = create_pipeline<fdtd2_push>(
    utils::get_engine_root_path() + "/modules/physics/shaders/spv/fdtd2_pressure_update.comp.spv",
    { vk_layout, vk_layout });

  velocity_pipeline_ = create_pipeline<fdtd2_push>(
    utils::get_engine_root_path() + "/modules/physics/shaders/spv/fdtd2_velocity_update.comp.spv",
    { vk_layout, vk_layout });
}

void fdtd2_compute_shader::render(const utils::compute_frame_info& info)
{
  if (target_ != nullptr) {
    auto &command = info.command_buffer;

    float dt = target_->get_dt();

    fdtd2_push push;
    push.x_grid = target_->get_x_grid();
    push.y_grid = target_->get_y_grid();
    push.x_len = target_->get_x_len();
    push.y_len = target_->get_y_len();
    push.v_fac = dt * target_->get_v_fac();
    push.p_fac = dt * target_->get_p_fac();

    auto reputation = static_cast<int>(info.dt / dt);

    target_->add_duration(dt * reputation);

    // update velocity and pressure
    for (int i = 0; i < reputation; i++) {
      // record pressure update 
      bind_pipeline(command);

      bind_push(command, VK_SHADER_STAGE_COMPUTE_BIT, push);

      auto desc_sets = target_->get_frame_desc_sets();
      bind_desc_sets(command, desc_sets);

      dispatch_command(
        command,
        (target_->get_x_grid() + fdtd2_local_size_x - 1) / fdtd2_local_size_x,
        (target_->get_y_grid() + fdtd2_local_size_y - 1) / fdtd2_local_size_y,
        1);
    }
  }
}

void fdtd2_compute_shader::set_target(fdtd2_field* target)
{ target_ = target; }

void fdtd2_compute_shader::remove_target(uint32_t field_id)
{ if (field_id == target_->get_field_id()) target_ = nullptr; }

} // namespace hnll::physics