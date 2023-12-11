// hnll
#include <graphics/desc_set.hpp>
#include "include/fdtd_cylindrical_field.hpp"
#include "include/fdtd_cylindrical_compute_shader.hpp"
#include "include/sine_oscillator.hpp"
#include <utils/singleton.hpp>

namespace hnll {

// shared with shaders
#include "common/fdtd_cylindrical.h"

// static member
fdtd_cylindrical_field* fdtd_cylindrical_compute_shader::target_ = nullptr;
uint32_t fdtd_cylindrical_compute_shader::target_id_ = -1;

fdtd_cylindrical_compute_shader::fdtd_cylindrical_compute_shader() : game::compute_shader<fdtd_cylindrical_compute_shader>()
{
  desc_layout_ = graphics::desc_layout::create_from_bindings(*device_, fdtd_cylindrical_field::field_bindings);
  auto vk_layout = desc_layout_->get_descriptor_set_layout();

  pipeline_ = create_pipeline<fdtd_cylindrical_push>(
    utils::get_engine_root_path() +
    "/examples/fdtd_compute/fdtd_cylindrical/shaders/spv/fdtd_cylindrical_compute.comp.spv",
    { vk_layout, vk_layout, vk_layout, vk_layout });
}

void fdtd_cylindrical_compute_shader::render(const utils::compute_frame_info& info)
{
  static sine_oscillator so;

  if (target_ != nullptr && target_->is_ready()) {
    auto &command = info.command_buffer;

    fdtd_cylindrical_push push;
    push.z_grid_count = target_->get_z_grid_count();
    push.r_grid_count = target_->get_r_grid_count();
    push.z_len = target_->get_z_len();
    push.r_len = target_->get_r_len();
    push.v_fac = target_->get_v_fac();
    push.p_fac = target_->get_p_fac();
    push.listener_index = target_->get_listener_index();
    push.input_pressure = target_->get_mouth_pressure();
    push.fcm_source_grid_id = target_->get_fcm_source_grid_id();

    bool fcm_on = push.fcm_source_grid_id != -1;
    so.set_freq(target_->get_fcm_freq());

    // barrier for pressure, velocity update synchronization
    VkMemoryBarrier barrier = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
      .pNext = nullptr,
      .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_SHADER_READ_BIT
    };

    {
      utils::scope_timer timer {"task dispatch"};
      // update velocity and pressure
      bind_pipeline(command);

      auto upf = target_->get_update_per_frame();
      for (int i = 0; i < upf; i++) {
        // record pressure update
        push.buffer_index = i;
        if (fcm_on) {
          push.input_pressure = 100000 * so.tick(DT);
        }

        bind_push(command, VK_SHADER_STAGE_COMPUTE_BIT, push);

        auto desc_sets = target_->get_frame_desc_sets();
        bind_desc_sets(command, desc_sets);

        dispatch_command(
          command,
          (target_->get_z_grid_count() + fdtd_cylindrical_local_size_x - 1) / fdtd_cylindrical_local_size_x,
          (target_->get_r_grid_count() + fdtd_cylindrical_local_size_y - 1) / fdtd_cylindrical_local_size_y,
          1
        );

        // if not the last loop, waite for velocity update
        if (i != upf - 1) {
          vkCmdPipelineBarrier(
            command,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_DEPENDENCY_DEVICE_GROUP_BIT,
            1, &barrier, 0, nullptr, 0, nullptr
          );
        }

        target_->update_frame();
      }
    }

    target_->update_sound_frame();
  }
}

void fdtd_cylindrical_compute_shader::set_target(fdtd_cylindrical_field* target)
{ target_ = target; }

void fdtd_cylindrical_compute_shader::remove_target(uint32_t target_id)
{ if (target_id_ == target_id) target_ = nullptr; }

} // namespace hnll