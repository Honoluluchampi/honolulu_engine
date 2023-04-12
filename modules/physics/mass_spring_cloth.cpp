// hnll
#include <physics/mass_spring_cloth.hpp>
#include <physics/compute_shader/cloth_compute_shader.hpp>
#include <physics/shading_system/cloth_compute_shading_system.hpp>
#include <graphics/desc_set.hpp>
#include <graphics/swap_chain.hpp>
#include <graphics/buffer.hpp>

namespace hnll::physics {

u_ptr<graphics::desc_layout> mass_spring_cloth::desc_layout_ = nullptr;

s_ptr<mass_spring_cloth> mass_spring_cloth::create()
{
  auto ret = std::make_shared<mass_spring_cloth>();

  // add to shaders
  cloth_compute_shader::add_cloth(ret);
  cloth_compute_shading_system::add_cloth(ret);

  return ret;
}

mass_spring_cloth::mass_spring_cloth() : device_(game::graphics_engine_core::get_device_r())
{
  static uint32_t id = 0;
  cloth_id_ = id++;

  setup_desc_sets();
}

void mass_spring_cloth::setup_desc_sets()
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
    vertex{vec3(0.5, 0.0, 0.0), vec3(0,0,0) },
    vertex{vec3(-0.5, 0.0, 0.0), vec3(0,0,0) },
    vertex{vec3(0.0, 0.0, 0.5), vec3(0,0,0) }
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

VkDescriptorSetLayout mass_spring_cloth::get_vk_desc_layout()
{ return desc_layout_->get_descriptor_set_layout(); }

void mass_spring_cloth::set_desc_layout()
{
  if (desc_layout_ == nullptr) {
    auto& device = game::graphics_engine_core::get_device_r();
    desc_layout_ = graphics::desc_layout::builder(device)
      .add_binding(
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT)
      .build();
  }
}

void mass_spring_cloth::reset_desc_layout()
{ desc_layout_.reset(); }

std::vector<VkDescriptorSet> mass_spring_cloth::get_vk_desc_sets(int frame_index)
{ return desc_sets_->get_vk_desc_sets(frame_index); }

} // namespace hnll::physics