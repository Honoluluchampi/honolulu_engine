// hnll
#include <physics/compute_shader/cloth_compute_shader.hpp>
#include <physics/mass_spring_cloth.hpp>
#include <graphics/swap_chain.hpp>
#include <graphics/buffer.hpp>
#include <graphics/desc_set.hpp>

namespace hnll::physics {

std::unordered_map<uint32_t, s_ptr<mass_spring_cloth>> cloth_compute_shader::clothes_;

DEFAULT_COMPUTE_SHADER_CTOR_IMPL(cloth_compute_shader);

void cloth_compute_shader::setup()
{
  mass_spring_cloth::set_desc_layout(device_);
  create_pipeline(
    utils::get_engine_root_path() + "/modules/physics/shaders/spv/cloth_compute.comp.spv",
    { mass_spring_cloth::get_vk_desc_layout() }
  );
}

void cloth_compute_shader::render(const utils::compute_frame_info &info)
{
  auto& command = info.command_buffer;

  bind_pipeline(command);

  for (auto& cloth_kv : clothes_) {
    auto& cloth = cloth_kv.second;
    bind_desc_sets(command, {cloth->get_vk_desc_sets(info.frame_index)});
    dispatch_command(command, 3, 1, 1);
  }
}

void cloth_compute_shader::add_cloth(const s_ptr<mass_spring_cloth> &cloth)
{ clothes_[cloth->get_id()] = cloth; }
} // namespace hnll::physics