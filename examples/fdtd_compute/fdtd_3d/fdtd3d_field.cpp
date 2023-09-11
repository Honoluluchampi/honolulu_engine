// hnll
#include "include/fdtd3_field.hpp"
#include <utils/singleton.hpp>
#include <graphics/device.hpp>
#include <graphics/buffer.hpp>
#include <utils/rendering_utils.hpp>

namespace hnll {

const std::vector<graphics::binding_info> fdtd3_field::field_bindings = {
  { VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER }
};

u_ptr<fdtd3_field> fdtd3_field::create(const hnll::fdtd_info &info)
{ return std::make_unique<fdtd3_field>(info); }

fdtd3_field::fdtd3_field(const hnll::fdtd_info &info)
  : device_(utils::singleton<graphics::device>::get_instance())
{
  length_ = info.length;
  c_ = info.sound_speed;
  rho_ = info.rho;
  pml_count_ = info.pml_count;
  update_per_frame_ = info.update_per_frame;

  compute_constants();
  setup_desc_sets(info);

  is_ready_ = true;
}

fdtd3_field::~fdtd3_field() {}

void fdtd3_field::compute_constants()
{
  dx_ = 3.83e-3;
  dt_ = 6.50e-6; // dt = dx / c / root(3)

  grid_count_.x() = std::ceil(length_.x() / dx_);
  grid_count_.y() = std::ceil(length_.y() / dx_);
  grid_count_.z() = std::ceil(length_.z() / dx_);

  v_fac_ = 1 / (rho_ * dx_);
  p_fac_ = rho_ * c_ * c_ / dx_;
}

void fdtd3_field::setup_desc_sets(const fdtd_info& info)
{
  desc_pool_ = graphics::desc_pool::builder(device_)
    .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frame_count_ * 6)
    .build();

  graphics::desc_set_info set_info { field_bindings };

  desc_sets_ = graphics::desc_sets::create(
    device_,
    desc_pool_,
    { set_info, set_info, set_info }, // filed and sound buffer
    frame_count_);

  // describe initial state here
  int grid_count = (grid_count_.x() + 1) * (grid_count_.y() + 1) * (grid_count_.z() + 1);
  std::vector<vec4> initial_grid(grid_count, { 0.f, 0.f, 0.f, 0.f });

  // assign buffer
  for  (int i = 0; i < frame_count_; i++) {
    auto press_buffer = graphics::buffer::create_with_staging(
      device_,
      sizeof(vec4) * initial_grid.size(),
      1,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      initial_grid.data()
    );

    auto active_buffer = graphics::buffer::create(
      device_,
      sizeof(int) * 256,
      1,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      nullptr
    );

    desc_sets_->set_buffer(0, 0, i, std::move(press_buffer));
    desc_sets_->set_buffer(2, 0, i, std::move(active_buffer));
  }

  std::vector<float> initial_sound_buffer;
  initial_sound_buffer.resize(update_per_frame_, 0.f);
  for (int i = 0; i < frame_count_; i++) {
    auto sound_buffer = graphics::buffer::create(
      device_,
      sizeof(float) * initial_sound_buffer.size(),
      1,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      initial_sound_buffer.data()
    );

    // preserve ptr for gpu buffer
    sound_buffers_[i] = reinterpret_cast<float*>(sound_buffer->get_mapped_memory());
    desc_sets_->set_buffer(1, 0, i, std::move(sound_buffer));
  }

  desc_sets_->build();
}

} // namespace hnll