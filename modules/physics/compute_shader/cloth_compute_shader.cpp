// hnll
#include <physics/compute_shader/cloth_compute_shader.hpp>
#include <physics/mass_spring_cloth.hpp>
#include <graphics/swap_chain.hpp>
#include <graphics/buffer.hpp>
#include <graphics/desc_set.hpp>

namespace hnll::physics {

// shared with shaders
#include "../common/cloth_config.h"

std::unordered_map<uint32_t, s_ptr<mass_spring_cloth>> cloth_compute_shader::clothes_;

DEFAULT_COMPUTE_SHADER_CTOR_IMPL(cloth_compute_shader);

cloth_compute_shader::~cloth_compute_shader()
{
  clothes_.clear();
  mass_spring_cloth::reset_desc_layout();
}

void cloth_compute_shader::setup()
{
  mass_spring_cloth::set_desc_layout();
  create_pipeline<cloth_push>(
    utils::get_engine_root_path() + "/modules/physics/shaders/spv/cloth_compute.comp.spv",
    { mass_spring_cloth::get_vk_desc_layout() }
  );
}

void cloth_compute_shader::render(const utils::compute_frame_info &info)
{
  auto& command = info.command_buffer;

  bind_pipeline(command);

  cloth_push push;
  push.dt = info.dt;

  for (auto& cloth_kv : clothes_) {
    auto& cloth = cloth_kv.second;
    push.x_grid = cloth->get_x_grid();
    push.y_grid = cloth->get_y_grid();
    bind_push(command, VK_SHADER_STAGE_COMPUTE_BIT, push);
    bind_desc_sets(command, {cloth->get_vk_desc_sets(info.frame_index)});
    dispatch_command(
      command,
      (cloth->get_x_grid() + cloth_local_size_x - 1) / cloth_local_size_x,
      (cloth->get_y_grid() + cloth_local_size_y - 1) / cloth_local_size_y,
      1);
  }
}

void cloth_compute_shader::add_cloth(const s_ptr<mass_spring_cloth> &cloth)
{ clothes_[cloth->get_id()] = cloth; }

void cloth_compute_shader::remove_cloth(uint32_t cloth_id)
{ clothes_.erase(cloth_id); }

} // namespace hnll::physics