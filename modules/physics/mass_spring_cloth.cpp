// hnll
#include <physics/mass_spring_cloth.hpp>
#include <physics/compute_shader/cloth_compute_shader.hpp>
#include <graphics/desc_set.hpp>
#include <graphics/swap_chain.hpp>
#include <graphics/buffer.hpp>

namespace hnll::physics {

VkDescriptorSetLayout mass_spring_cloth::vk_desc_layout_ = VK_NULL_HANDLE;

s_ptr<mass_spring_cloth> mass_spring_cloth::create(graphics::device& device)
{
  auto ret = std::make_shared<mass_spring_cloth>(device);

  // add to shaders
  cloth_compute_shader::add_cloth(ret);

  return ret;
}

mass_spring_cloth::mass_spring_cloth(graphics::device& device)
{
  static uint32_t id = 0;
  cloth_id_ = id++;

  setup_desc_sets(device);
}

void mass_spring_cloth::setup_desc_sets(graphics::device& device)
{
  int frame_in_flight = graphics::swap_chain::MAX_FRAMES_IN_FLIGHT;
  // pool
  desc_pool_ = graphics::desc_pool::builder(device)
    .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frame_in_flight)
    .build();

  // build desc sets
  graphics::desc_set_info set_info;
  set_info.add_binding(
    VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT,
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
  set_info.is_frame_buffered_ = true;

  desc_sets_ = graphics::desc_sets::create(
    device,
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
      device,
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

void mass_spring_cloth::set_vk_desc_layout(graphics::device& device)
{
  if (vk_desc_layout_ == VK_NULL_HANDLE) {
    auto desc_layout = graphics::desc_layout::builder(device)
      .add_binding(
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT)
      .build();
    vk_desc_layout_ = desc_layout->get_descriptor_set_layout();
  }
}

std::vector<VkDescriptorSet> mass_spring_cloth::get_vk_desc_sets(int frame_index)
{ return desc_sets_->get_vk_desc_sets(frame_index); }

} // namespace hnll::physics