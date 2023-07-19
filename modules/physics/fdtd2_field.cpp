// hnll
#include <game/modules/graphics_engine.hpp>
#include <physics/fdtd2_field.hpp>
#include <physics/compute_shader/fdtd2_compute_shader.hpp>
#include <physics/shading_system/fdtd2_shading_system.hpp>

// std
#include <thread>

namespace hnll::physics {

#include "common/fdtd_struct.h"

// only binding of the pressure is accessed by fragment shader
const std::vector<graphics::binding_info> fdtd2_field::field_bindings = {
  {VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER }
};

const std::vector<graphics::binding_info> fdtd2_field::texture_bindings = {
  {VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}
};

u_ptr<fdtd2_field> fdtd2_field::create(const fdtd_info& info)
{ return std::make_unique<fdtd2_field>(info); }

fdtd2_field::fdtd2_field(const fdtd_info& info) : device_(game::graphics_engine_core::get_device_r())
{
  static uint32_t id = 0;
  field_id_ = id++;

  x_len_ = info.x_len;
  y_len_ = info.y_len;
  c_ = info.sound_speed;
  rho_ = info.rho;
  pml_count_ = info.pml_count;

  compute_constants();
  setup_desc_sets(info);

  is_ready_ = true;
}

fdtd2_field::~fdtd2_field()
{
  fdtd2_compute_shader::remove_target(id_);
  fdtd2_shading_system::remove_target(id_);
}

void fdtd2_field::compute_constants()
{
  dx_ = 3.83e-3;
  dt_ = 7.81e-6;

  x_grid_ = std::ceil(x_len_ / dx_);
  y_grid_ = std::ceil(y_len_ / dx_);

  v_fac_ = 1 / (rho_ * dx_);
  p_fac_ = rho_ * c_ * c_ / dx_;
}

void fdtd2_field::setup_desc_sets(const fdtd_info& info)
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
  int grid_count = (x_grid_ + 1) * (y_grid_ + 1);
  std::vector<particle> initial_grid(grid_count, {.values = {0.f, 0.f, 0.f, 0.f}});

  // set pml value
  float pml_each = 0.5f / float(pml_count_);
  for (int i = 0; i < grid_count; i++) {
    // retrieve coordinate
    auto x = float(i % (x_grid_ + 1)) - 1;
    auto y = float(i / (x_grid_ + 1)) - 1;
    float pml_l = std::max(float(pml_count_) - x, 0.f) * pml_each;
    float pml_r = std::max(float(pml_count_) - (x_grid_ - x), 0.f) * pml_each;
    float pml_d = std::max(float(pml_count_) - y, 0.f) * pml_each;
    float pml_u = std::max(float(pml_count_) - (y_grid_ - y), 0.f) * pml_each;
    auto pml_x = std::max(pml_l, pml_r);
    auto pml_y = std::max(pml_u, pml_d);
    auto pml = std::max(pml_x, pml_y);
    initial_grid[i].values.w() = pml;
  }

  int impulse_grid_x = info.x_impulse / info.x_len * (x_grid_ + 1);
  int impulse_grid_y = info.y_impulse / info.y_len * (y_grid_ + 1);
  int impulse_grid_id = impulse_grid_x + impulse_grid_y * (x_grid_ + 1);

  // assign buffer
  for (int i = 0; i < frame_count_; i++) {
//    // setup initial pressure as impulse signal from the center of the room
//    if (i == 0)
//      initial_grid[impulse_grid_id].values.z() = 128.f;

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

void fdtd2_field::setup_textures(const fdtd_info& info)
{
  // create desc sets
  desc_pool_ = graphics::desc_pool::builder(device_)
    .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, frame_count_)
    .build();

  auto desc_layout = graphics::desc_layout::builder(device_)
    .add_binding(
      VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
      VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
    .build();

  // create initial data
  int grid_count = (x_grid_ + 1) * (y_grid_ + 1);
  std::vector<vec4> initial_grid(grid_count, vec4{0.f, 0.f, 0.f, 0.f});

  int impulse_grid_x = info.x_impulse / info.x_len * (x_grid_ + 1);
  int impulse_grid_y = info.y_impulse / info.y_len * (y_grid_ + 1);
  int impulse_grid_id = impulse_grid_x + impulse_grid_y * (x_grid_ + 1);

  // assign buffer
  for (int i = 0; i < frame_count_; i++) {
    auto initial_buffer = graphics::buffer::create(
      device_,
      4 * initial_grid.size(),
      1,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      initial_grid.data());

    // prepare image resource
    VkExtent3D extent = { static_cast<uint32_t>(x_grid_ + 1), static_cast<uint32_t>(y_grid_ + 1), 1 };
    auto image = graphics::image_resource::create(
      device_,
      extent,
      VK_FORMAT_R32G32B32A32_SFLOAT,
      VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    image->transition_image_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    image->copy_buffer_to_image(initial_buffer->get_buffer(), extent);

    image->transition_image_layout(VK_IMAGE_LAYOUT_GENERAL);

    VkDescriptorImageInfo image_info = {
      .sampler = nullptr,
      .imageView = image->get_image_view(),
      .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };

    graphics::desc_writer(*desc_layout, *desc_pool_)
      .write_image(0, &image_info)
      .build(texture_vk_desc_sets_[i]);
  }
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

void fdtd2_field::set_as_target(fdtd2_field* target) const
{
  fdtd2_compute_shader::set_target(target);
  fdtd2_shading_system::set_target(target);
}

} // namespace hnll::physics