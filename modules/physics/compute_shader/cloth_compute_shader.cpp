// hnll
#include <physics/compute_shader/cloth_compute_shader.hpp>
#include <graphics/swap_chain.hpp>
#include <graphics/buffer.hpp>
#include <graphics/desc_set.hpp>

namespace hnll::physics {

DEFAULT_COMPUTE_SHADER_CTOR_IMPL(cloth_compute_shader);

void cloth_compute_shader::setup()
{
  setup_desc_sets();
  create_pipeline(
    utils::get_engine_root_path() + "/modules/physics/shaders/spv/cloth_compute.comp.spv",
    {desc_sets_->get_vk_layouts()[0]}
  );
}

void cloth_compute_shader::render(const utils::compute_frame_info &info)
{
  auto& command = info.command_buffer;

  bind_pipeline(command);
  bind_desc_sets(command, {desc_sets_->get_vk_desc_sets(info.frame_index)});
  dispatch_command(command, 3, 1, 1);
}

void cloth_compute_shader::setup_desc_sets()
{
  int frame_in_flight = graphics::swap_chain::MAX_FRAMES_IN_FLIGHT;
  // pool
  desc_pool_ = graphics::desc_pool::builder(device_)
    .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frame_in_flight)
    .build();

  // build desc sets
  graphics::desc_set_info set_info;
  set_info.add_binding(
    VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT,
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
  set_info.is_frame_buffered_ = true;

  desc_sets_ = graphics::desc_sets::create(
    device_,
    desc_pool_,
    {set_info},
    graphics::swap_chain::MAX_FRAMES_IN_FLIGHT);

  // create initial data
  std::array<vertex, 3> vertices = {
    vec3{1.f, -1.f, 0.f},
    vec3{-1.f, -1.f, 0.f},
    vec3{0.f, -1.f, 0.f}
  };

  // assign buffer
  for (int i = 0; i < frame_in_flight; i++) {
    auto new_buffer = graphics::buffer::create_with_staging(
      device_,
      sizeof(vertex) * 3,
      1,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      vertices.data()
    );
    desc_sets_->set_buffer(0, 0, i, std::move(new_buffer));
  }

  desc_sets_->build();
}

} // namespace hnll::physics