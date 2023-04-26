// hnll
#include <graphics/desc_set.hpp>
#include <physics/fdtd2_field.hpp>
#include <physics/compute_shader/fdtd2_compute_shader.hpp>
#include <utils/singleton.hpp>

namespace hnll::physics {

// shared with shaders
#include "../common/fdtd2_config.h"

DEFAULT_COMPUTE_SHADER_CTOR_IMPL(fdtd2_compute_shader);

void fdtd2_compute_shader::setup()
{
  layout_ = graphics::desc_layout::create_from_bindings(device_, fdtd2_field::field_bindings);
  auto vk_layout = layout_->get_descriptor_set_layout();
  create_pipeline<fdtd2_push>(
    utils::get_engine_root_path() + "modules/physics/shaders/spv/fdtd2.comp.spv",
    { vk_layout, vk_layout });
}

void fdtd2_compute_shader::render(const utils::compute_frame_info& info)
{
  auto& command = info.command_buffer;
  bind_pipeline(command);

  fdtd2_push push;

}

void fdtd2_compute_shader::set_target(const s_ptr<fdtd2_field> &target)
{ target_ = target; }

void fdtd2_compute_shader::remove_target(uint32_t field_id)
{ if (field_id == target_->get_field_id()) target_.reset(); }

} // namespace hnll::physics