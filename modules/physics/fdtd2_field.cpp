// hnll
#include <game/modules/graphics_engine.hpp>
#include <physics/fdtd2_field.hpp>

namespace hnll::physics {

// only binding of the pressure is accessed by fragment shader
const std::vector<graphics::binding_info> fdtd2_field::field_bindings = {
  {VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
  {VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
  {VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
};

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
    .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frame_count_)
    .build();

  graphics::desc_set_info set_info { field_bindings };
  set_info.is_frame_buffered_ = true;

  desc_sets_ = graphics::desc_sets::create(
    device_,
    desc_pool_,
    {set_info},
    frame_count_);

  // initial data
  int press_grid_count = x_grid_ * y_grid_;
  int vx_grid_count = (x_grid_ + 1) * y_grid_;
  int vy_grid_count = x_grid_ * (y_grid_ + 1);
  std::vector<float> initial_press(press_grid_count, 0.f);
  std::vector<float> initial_vx(vx_grid_count, 0.f);
  std::vector<float> initial_vy(vy_grid_count, 0.f);

  // assign buffer
  for (int i = 0; i < frame_count_; i++) {
    // setup initial pressure as impulse signal from the center of the room
    int center_grid_id = x_grid_ / 2 + y_grid_ / 2 * x_grid_;
    if (i == 0) {
      initial_press[center_grid_id] = 500.f;
    }
    else {
      initial_press[center_grid_id] = 0.f;
    }

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

  frame_index_ = frame_index_ == 0 ? 1 : 0;

  return desc_sets;
}

} // namespace hnll::physics