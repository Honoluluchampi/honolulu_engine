// hnll
#include <game/modules/graphics_engine.hpp>
#include "include/fdtd_cylindrical_field.hpp"
#include "include/fdtd_cylindrical_compute_shader.hpp"
#include "include/fdtd_cylindrical_shading_system.hpp"

// std
#include <thread>

namespace hnll {

// only binding of the pressure is accessed by fragment shader
const std::vector<graphics::binding_info> fdtd_cylindrical_field::field_bindings = {
  {VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER }
};

u_ptr<fdtd_cylindrical_field> fdtd_cylindrical_field::create(const fdtd_info& info)
{ return std::make_unique<fdtd_cylindrical_field>(info); }

fdtd_cylindrical_field::fdtd_cylindrical_field(const fdtd_info& info)
  : device_(utils::singleton<graphics::device>::get_single_ptr())
{
  static uint32_t id = 0;
  field_id_ = id++;

  z_len_ = info.z_len;
  r_len_ = info.r_len;
  c_ = info.sound_speed;
  rho_ = info.rho;
  pml_count_ = info.pml_count;
  update_per_frame_ = info.update_per_frame;

  compute_constants();
  setup_desc_sets(info);

  is_ready_ = true;
}

fdtd_cylindrical_field::~fdtd_cylindrical_field()
{
  fdtd_cylindrical_compute_shader::remove_target(id_);
  fdtd_cylindrical_shading_system::remove_target(id_);
}

void fdtd_cylindrical_field::compute_constants()
{
  dz_ = 3.83e-3;
  dr_ = 3.83e-3;
  dt_ = 7.81e-6;

  z_grid_count_ = std::ceil(z_len_ / dz_) + 1 + pml_count_ * 2;
  r_grid_count_ = std::ceil(r_len_ / dr_) + pml_count_;

  v_fac_ = dt_ / rho_;
  p_fac_ = dt_ * rho_ * c_ * c_;
}

#include "common/fdtd_cylindrical.h"

void fdtd_cylindrical_field::set_pml(
  std::vector<particle>& grids,
  int z_min, int z_max, int r_max)
{
  float pml_each = 0.5f / float(pml_count_);

  for (int z = z_min; z <= z_max; z++) {
    for (int r = 0; r <= r_max; r++) {
      float pml_l = std::max(float(pml_count_ - (z - z_min)), 0.f) * pml_each;
      float pml_r = std::max(float(pml_count_ - (z_max - z)), 0.f) * pml_each;
      float pml_y = std::max(float(pml_count_ - (r_max - r)), 0.f) * pml_each;
      auto pml_x = std::max(pml_l, pml_r);
      auto pml = std::max(pml_x, pml_y);
      auto idx = z + (z_grid_count_ + 1) * r;
      grids[idx].state = pml;
    }
  }
}

void fdtd_cylindrical_field::setup_desc_sets(const fdtd_info& info)
{
  desc_pool_ = graphics::desc_pool::builder(*device_)
    .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frame_count_ * 12)
    .build();

  graphics::desc_set_info set_info { field_bindings };

  desc_sets_ = graphics::desc_sets::create(
    *device_,
    desc_pool_,
    {set_info, set_info, set_info, set_info}, // field and sound buffer
    frame_count_);

  // initial data
  int grid_count = z_grid_count_ * r_grid_count_;
  std::vector<particle> initial_grid(grid_count, {0.f, 0.f, 0.f, 0.f});

  set_pml(initial_grid, 0, z_grid_count_, r_grid_count_);

  // assign buffer
  for (int i = 0; i < frame_count_; i++) {

    auto particle_buffer = graphics::buffer::create_with_staging(
      *device_,
      sizeof(particle) * initial_grid.size(),
      1,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      initial_grid.data());

    desc_sets_->set_buffer(0, 0, i, std::move(particle_buffer));
  }

  std::vector<float> initial_sound_buffer;
  initial_sound_buffer.resize(update_per_frame_, 0.f);
  for (int i = 0; i < frame_count_; i++) {
    auto sound_buffer = graphics::buffer::create(
      *device_,
      sizeof(float) * initial_sound_buffer.size(),
      1,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      initial_sound_buffer.data()
    );

    sound_buffers_[i] = reinterpret_cast<float*>(sound_buffer->get_mapped_memory());

    desc_sets_->set_buffer(1, 0, i, std::move(sound_buffer));
  }

  desc_sets_->build();
}

std::vector<VkDescriptorSet> fdtd_cylindrical_field::get_frame_desc_sets()
{
  if (frame_index_ == 0) {
    return {
      desc_sets_->get_vk_desc_sets(0)[0],
      desc_sets_->get_vk_desc_sets(2)[0],
      desc_sets_->get_vk_desc_sets(1)[0],
      desc_sets_->get_vk_desc_sets(0)[2]
    };
  }
  else if (frame_index_ == 1){
    return {
      desc_sets_->get_vk_desc_sets(1)[0],
      desc_sets_->get_vk_desc_sets(0)[0],
      desc_sets_->get_vk_desc_sets(2)[0],
      desc_sets_->get_vk_desc_sets(1)[2]
    };
  }
  else {
    return {
      desc_sets_->get_vk_desc_sets(2)[0],
      desc_sets_->get_vk_desc_sets(1)[0],
      desc_sets_->get_vk_desc_sets(0)[0],
      desc_sets_->get_vk_desc_sets(2)[2]
    };
  }
}

void fdtd_cylindrical_field::set_as_target(fdtd_cylindrical_field* target) const
{
  fdtd_cylindrical_compute_shader::set_target(target);
  fdtd_cylindrical_shading_system::set_target(target);
}

} // namespace hnll