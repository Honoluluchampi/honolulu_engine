// hnll
#include <game/modules/graphics_engine.hpp>
#include <physics/fdtd2_field.hpp>
#include <graphics/desc_set.hpp>

namespace hnll::physics {

u_ptr<fdtd2_field> fdtd2_field::create(const fdtd_info& info)
{ return std::make_unique<fdtd2_field>(info); }

fdtd2_field::fdtd2_field(const fdtd_info& info) : device_(game::graphics_engine_core::get_device_r())
{
  x_len_ = info.x_len;
  y_len_ = info.y_len;
  sound_speed_ = info.sound_speed;
  kappa_ = info.sound_speed;
  rho_ = info.sound_speed;
  f_max_ = info.f_max;

  compute_constants();
  setup_desc_sets();
}

void fdtd2_field::compute_constants()
{
  dt_ = 1 / (2 * f_max_);
  grid_size_ = std::sqrt(2) * sound_speed_ * dt_;

  x_grid_ = std::ceil(x_len_ / grid_size_);
  y_grid_ = std::ceil(y_len_ / grid_size_);
}

void fdtd2_field::setup_desc_sets()
{
  desc_pool_ = graphics::desc_pool::builder(device_)
    .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, utils::FRAMES_IN_FLIGHT)
    .build();

  graphics::desc_set_info set_info;
  // 3 bindings for pressure, x-velocity, y-velocity
  for (int i = 0; i < 3; i++) {
    set_info.add_binding(
      VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
  }
  set_info.is_frame_buffered_ = true;

  desc_sets_ = graphics::desc_sets::create(
    device_,
    desc_pool_,
    {set_info},
    utils::FRAMES_IN_FLIGHT);

  // initial data
  int press_grid_count = x_grid_ * y_grid_;
  int vx_grid_count = (x_grid_ + 1) * y_grid_;
  int vy_grid_count = x_grid_ * (y_grid_ + 1);
  std::vector<float> initial_press(0, press_grid_count);
  std::vector<float> initial_vx(0, vx_grid_count);
  std::vector<float> initial_vy(0, vy_grid_count);

  // assign buffer
  for (int i = 0; i < utils::FRAMES_IN_FLIGHT; i++) {
    auto press_buffer = graphics::buffer::create_with_staging(
      device_,
      sizeof(float) * press_grid_count,
      1,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      nullptr);
    auto vx_buffer = graphics::buffer::create_with_staging(
      device_,
      sizeof(float) * vx_grid_count,
      1,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      nullptr);
    auto vy_buffer = graphics::buffer::create_with_staging(
      device_,
      sizeof(float) * vy_grid_count,
      1,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      nullptr);

    desc_sets_->set_buffer(0, 0, i, std::move(press_buffer));
    desc_sets_->set_buffer(0, 1, i, std::move(vx_buffer));
    desc_sets_->set_buffer(0, 2, i, std::move(vy_buffer));
  }

  desc_sets_->build();
}

} // namespace hnll::physics