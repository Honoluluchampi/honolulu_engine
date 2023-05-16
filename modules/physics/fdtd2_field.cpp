// hnll
#include <game/modules/graphics_engine.hpp>
#include <physics/fdtd2_field.hpp>
#include <physics/compute_shader/fdtd2_compute_shader.hpp>
#include <physics/shading_system/fdtd2_shading_system.hpp>

// std
#include <thread>

namespace hnll::physics {

//#include "common/fdtd_struct.h"
struct particle {
  int state;
  alignas(16) vec3 values; // x : vx, y : vy, z : pressure
//  int state; // 0 : fixed, 1 : free
};
// only binding of the pressure is accessed by fragment shader
const std::vector<graphics::binding_info> fdtd2_field::field_bindings = {
  {VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER }
};

u_ptr<fdtd2_field> fdtd2_field::create(const fdtd_info& info)
{
  auto ret = std::make_unique<fdtd2_field>(info);
  fdtd2_compute_shader::set_target(ret.get());
  fdtd2_shading_system::set_target(ret.get());
  return ret;
}

fdtd2_field::fdtd2_field(const fdtd_info& info) : device_(game::graphics_engine_core::get_device_r())
{
  static uint32_t id = 0;
  field_id_ = id++;

  x_len_ = info.x_len;
  y_len_ = info.y_len;
  sound_speed_ = info.sound_speed;
  kappa_ = info.kappa;
  rho_ = info.rho;
  f_max_ = info.f_max;

  compute_constants();
  setup_desc_sets(info);

  is_ready_ = true;
}

fdtd2_field::~fdtd2_field()
{
  fdtd2_compute_shader::remove_target(field_id_);
  fdtd2_shading_system::remove_target(field_id_);
}

void fdtd2_field::compute_constants()
{
  grid_size_ = sound_speed_ / (2 * f_max_) / 10.f;
  dt_ = grid_size_ / sound_speed_;

  x_grid_ = std::ceil(x_len_ / grid_size_);
  y_grid_ = std::ceil(y_len_ / grid_size_);
}

void fdtd2_field::setup_desc_sets(const fdtd_info& info)
{
  desc_pool_ = graphics::desc_pool::builder(device_)
    .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frame_count_ * 3)
    .build();

  graphics::desc_set_info set_info { field_bindings };
  set_info.is_frame_buffered_ = true;

  desc_sets_ = graphics::desc_sets::create(
    device_,
    desc_pool_,
    {set_info},
    frame_count_);

  // initial data
  int grid_count = (x_grid_ + 1) * (y_grid_ + 1);
  std::vector<particle> initial_grid(grid_count, {.values = {0.f, 0.f, 0.f}});

  int impulse_grid_x = info.x_impulse / info.x_len * (x_grid_ + 1);
  int impulse_grid_y = info.y_impulse / info.y_len * (y_grid_ + 1);
  int impulse_grid_id = impulse_grid_x + impulse_grid_y * (x_grid_ + 1);

  // assign buffer
  for (int i = 0; i < frame_count_; i++) {
    // setup initial pressure as impulse signal from the center of the room
    if (i == 1)
      initial_grid[impulse_grid_id].values.z() = 128.f;

    auto press_buffer = graphics::buffer::create_with_staging(
      device_,
      sizeof(particle) * initial_grid.size(),
      1,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      initial_grid.data());

    desc_sets_->set_buffer(0, 0, i, std::move(press_buffer));
  }

  desc_sets_->build();
}

std::vector<VkDescriptorSet> fdtd2_field::get_frame_desc_sets()
{
  std::vector<VkDescriptorSet> desc_sets;
  if (frame_index_ == 0) {
    desc_sets = {
      desc_sets_->get_vk_desc_sets(0)[0],
      desc_sets_->get_vk_desc_sets(1)[0]
    };
  }
  else {
    desc_sets = {
      desc_sets_->get_vk_desc_sets(1)[0],
      desc_sets_->get_vk_desc_sets(0)[0]
    };
  }

  return desc_sets;
}

} // namespace hnll::physics