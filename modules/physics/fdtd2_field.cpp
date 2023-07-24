// hnll
#include <game/modules/graphics_engine.hpp>
#include <physics/fdtd2_field.hpp>
#include <physics/compute_shader/fdtd2_compute_shader.hpp>
#include <physics/shading_system/fdtd2_shading_system.hpp>

// std
#include <thread>

namespace hnll::physics {

// only binding of the pressure is accessed by fragment shader
const std::vector<graphics::binding_info> fdtd2_field::field_bindings = {
  {VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER }
};

const std::vector<graphics::binding_info> fdtd2_field::texture_bindings = {
  {VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}
};

u_ptr<fdtd2_field> fdtd2_field::create(const fdtd_info& info)
{ return std::make_unique<fdtd2_field>(info); }

fdtd2_field::fdtd2_field(const fdtd_info& info)
  : device_(utils::singleton<graphics::device>::get_instance())
{
  static uint32_t id = 0;
  field_id_ = id++;

  x_len_ = info.x_len;
  y_len_ = info.y_len;
  c_ = info.sound_speed;
  rho_ = info.rho;
  pml_count_ = info.pml_count;
  update_per_frame_ = info.update_per_frame;

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

void fdtd2_field::set_pml(
  std::vector<vec4>& grids,
  std::vector<int>& is_active,
  int x_min, int x_max, int y_min, int y_max)
{
  float pml_each = 0.5f / float(pml_count_);

  for (int x = x_min; x <= x_max; x++) {
    for (int y = y_min; y <= y_max; y++) {
      float pml_l = std::max(float(pml_count_ - (x - x_min)), 0.f) * pml_each;
      float pml_r = std::max(float(pml_count_ - (x_max - x)), 0.f) * pml_each;
      float pml_d = std::max(float(pml_count_ - (y - y_min)), 0.f) * pml_each;
      float pml_u = std::max(float(pml_count_ - (y_max - y)), 0.f) * pml_each;
      auto pml_x = std::max(pml_l, pml_r);
      auto pml_y = std::max(pml_u, pml_d);
      auto pml = std::max(pml_x, pml_y);
      auto idx = x + (x_grid_ + 1) * y;
      grids[idx].w() = pml;
      is_active[idx] = 1.f;
    }
  }
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
    {set_info, set_info, set_info}, // field and sound buffer
    frame_count_);

  // initial data
  int grid_count = (x_grid_ + 1) * (y_grid_ + 1);
  std::vector<vec4> initial_grid(grid_count, {0.f, 0.f, 0.f, 0.f});
  std::vector<int> is_active(grid_count, 0.f);

  set_pml(initial_grid, is_active, 115, 145, 25, 53);
  set_pml(initial_grid, is_active, 70, 114, 20, 42);
  set_pml(initial_grid, is_active, 0, x_grid_, 0, y_grid_);

  // set state
  for (int i = 0; i < grid_count; i++) {
    // retrieve coordinate
    auto x = float(i % (x_grid_ + 1)) - 1;
    auto y = float(i / (x_grid_ + 1)) - 1;

    std::vector<int> active_grid_ids;

    // temp
    // state (wall, exciter)
    if (x >= 25 && x <= 130) {
      if (y > 36 && y < 42) {
        initial_grid[i].w() = 0.f;
        is_active[i] = 1.f;
      }
      if (y == 36 || y == 42) {
      initial_grid[i].w() = -2; // wall
//      if (((x == 86) || (x >= 100 && x <= 101)) && y == 36)
//        initial_grid[i].w() = 0; // tone hole
      }
    }
    if ((x == 25) && (y > 36 && y < 42)) {
      initial_grid[i].w() = -3; // exciter
      is_active[i] = 1.f;
    }
    if ((x == 138) && (y == 39)) {
      initial_grid[i].w() = -1;
      listener_index_ = i;
    }
  }

  // assign buffer
  for (int i = 0; i < frame_count_; i++) {

    auto press_buffer = graphics::buffer::create_with_staging(
      device_,
      sizeof(vec4) * initial_grid.size(),
      1,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      initial_grid.data());

    auto is_active_buffer = graphics::buffer::create_with_staging(
      device_,
      sizeof(int) * is_active.size(),
      1,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      is_active.data()
    );

    desc_sets_->set_buffer(0, 0, i, std::move(press_buffer));
    desc_sets_->set_buffer(2, 0, i, std::move(is_active_buffer));
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

    sound_buffers_[i] = reinterpret_cast<float*>(sound_buffer->get_mapped_memory());

    desc_sets_->set_buffer(1, 0, i, std::move(sound_buffer));
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

std::vector<VkDescriptorSet> fdtd2_field::get_frame_desc_sets(int game_frame_index)
{
  std::vector<VkDescriptorSet> desc_sets;
  if (frame_index_ == 0) {
    desc_sets = {
      desc_sets_->get_vk_desc_sets(0)[0],
      desc_sets_->get_vk_desc_sets(2)[0],
      desc_sets_->get_vk_desc_sets(1)[0],
      desc_sets_->get_vk_desc_sets(game_frame_index)[1],
      desc_sets_->get_vk_desc_sets(0)[2]
    };
  }
  else if (frame_index_ == 1){
    desc_sets = {
      desc_sets_->get_vk_desc_sets(1)[0],
      desc_sets_->get_vk_desc_sets(0)[0],
      desc_sets_->get_vk_desc_sets(2)[0],
      desc_sets_->get_vk_desc_sets(game_frame_index)[1],
      desc_sets_->get_vk_desc_sets(1)[2]
    };
  }
  else {
    desc_sets = {
      desc_sets_->get_vk_desc_sets(2)[0],
      desc_sets_->get_vk_desc_sets(1)[0],
      desc_sets_->get_vk_desc_sets(0)[0],
      desc_sets_->get_vk_desc_sets(game_frame_index)[1],
      desc_sets_->get_vk_desc_sets(2)[2]
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