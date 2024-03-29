// hnll
#include <physics/mass_spring_cloth.hpp>
#include <physics/resource_manager.hpp>
#include <game/modules/graphics_engine.hpp>
#include <graphics/desc_set.hpp>
#include <graphics/buffer.hpp>

namespace hnll::physics {

u_ptr<graphics::desc_layout> mass_spring_cloth::desc_layout_ = nullptr;

s_ptr<mass_spring_cloth> mass_spring_cloth::create(int x_grid, int y_grid, float x_len, float y_len)
{
  auto ret = std::make_shared<mass_spring_cloth>(x_grid, y_grid, x_len, y_len);

  // add to shaders
  resource_manager::add_cloth(ret);

  return ret;
}

mass_spring_cloth::mass_spring_cloth(int x_grid, int y_grid, float x_len, float y_len)
{
  static uint32_t id = 0;
  cloth_id_ = id++;

  assert((x_grid % 2 == 0 && y_grid % 2 == 0) && "mass_spring_cloth : each grid count should be even number.");
  x_grid_ = x_grid;
  y_grid_ = y_grid;
  x_len_ = x_len;
  y_len_ = y_len;
  indices_count_ = (x_grid_ - 1) * (y_grid_ - 1) * 6;

  auto mesh = construct_mesh(x_len, y_len);
  auto indices = construct_index_buffer();
  setup_desc_sets(std::move(mesh), std::move(indices));
}

mass_spring_cloth::~mass_spring_cloth()
{ resource_manager::remove_cloth(cloth_id_); }

std::vector<vertex> mass_spring_cloth::construct_mesh(float x_len, float y_len)
{
  int grid_count = x_grid_ * y_grid_;
  std::vector<vertex> mesh(grid_count);

  // temporal value
  vec3 center = vec3(0.f, -3.f, 0.f);
  vec3 initial_velocity = vec3(0.f, 0.f, 0.f);
  vec3 initial_normal = vec3(0.f, -1.f, 0.f);

  float x_grid_len = x_len / static_cast<float>(x_grid_ - 1);
  float y_grid_len = y_len / static_cast<float>(y_grid_ - 1);

  // calc initial position of each vertex;
  for (int j = 0; j < y_grid_; j++) {
    for (int i = 0; i < x_grid_; i++) {
      int id = i + x_grid_ * j;
      vec3 position = {
        center.x() + x_grid_len * (i - static_cast<float>(x_grid_ - 1) / 2.f),
        center.y(),
        center.z() + y_grid_len * (j - static_cast<float>(y_grid_ - 1) / 2.f)
      };

      mesh[id] = vertex { position, initial_velocity, initial_normal };
    }
  }

  return mesh;
}

std::vector<uint32_t> mass_spring_cloth::construct_index_buffer()
{
  std::vector<uint32_t> ret;
  // each rectangle consists of 6 indices
  ret.resize((x_grid_ - 1) * (y_grid_ - 1) * 6);

  int id = 0;
  // can be determined by only grid info
  for (int j = 0; j < y_grid_ - 1; j++) {
    for (int i = 0; i < x_grid_ - 1; i++) {
      ret[id++] = i + x_grid_ * j;
      ret[id++] = i + 1 + x_grid_ * j;
      ret[id++] = i + 1 + x_grid_ * (j + 1);
      ret[id++] = i + x_grid_ * j;
      ret[id++] = i + x_grid_ * (j + 1);
      ret[id++] = i + 1 + x_grid_ * (j + 1);
    }
  }
  assert(id == ret.size() && "mass_spring_cloth : indexing failure.");

  return ret;
}

void mass_spring_cloth::setup_desc_sets(std::vector<vertex>&& mesh, std::vector<uint32_t>&& indices)
{
  auto device = (utils::singleton<graphics::device>::get_single_ptr());

  // for central difference approximation
  int frame_in_flight = 3;
  vertex_buffers_.resize(frame_in_flight);

  // pool
  desc_pool_ = graphics::desc_pool::builder(*device)
    .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frame_in_flight)
    .build();

  // build desc sets
  graphics::desc_set_info set_info;
  // vertex buffer
  set_info.add_binding(
    VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT,
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

  desc_sets_ = graphics::desc_sets::create(
    *device,
    desc_pool_,
    {set_info},
    frame_in_flight);

  // assign buffer
  for (int i = 0; i < frame_in_flight; i++) {
    // vertex buffer
    auto vertex_buffer = graphics::buffer::create_with_staging(
      *device,
      sizeof(vertex) * mesh.size(),
      1,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      mesh.data()
    );

    vertex_buffers_[i] = vertex_buffer.get();

    desc_sets_->set_buffer(0, 0, i, std::move(vertex_buffer));
  }

  desc_sets_->build();

  // index buffer
  index_buffer_ = graphics::buffer::create_with_staging(
    *device,
    sizeof(uint32_t) * indices.size(),
    1,
    VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    indices.data()
  );
}

VkDescriptorSetLayout mass_spring_cloth::get_vk_desc_layout()
{ return desc_layout_->get_descriptor_set_layout(); }

void mass_spring_cloth::set_desc_layout()
{
  if (desc_layout_ == nullptr) {
    auto device = (utils::singleton<graphics::device>::get_single_ptr());
    desc_layout_ = graphics::desc_layout::builder(*device)
      // vertex buffer
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

std::vector<VkDescriptorSet> mass_spring_cloth::get_frame_desc_sets() const
{
  // poll desc sets (each frame has only one desc sets)
  std::vector<VkDescriptorSet> desc_sets;
  if (frame_index_ == 0) {
    desc_sets = {
      desc_sets_->get_vk_desc_sets(0)[0],
      desc_sets_->get_vk_desc_sets(2)[0],
      desc_sets_->get_vk_desc_sets(1)[0]
    };
  }
  else if (frame_index_ == 1) {
    desc_sets = {
      desc_sets_->get_vk_desc_sets(1)[0],
      desc_sets_->get_vk_desc_sets(0)[0],
      desc_sets_->get_vk_desc_sets(2)[0]
    };
  }
  else {
    desc_sets = {
      desc_sets_->get_vk_desc_sets(2)[0],
      desc_sets_->get_vk_desc_sets(1)[0],
      desc_sets_->get_vk_desc_sets(0)[0]
    };
  }
  return desc_sets;
}

void mass_spring_cloth::bind(VkCommandBuffer cb)
{
  VkBuffer buffers[] = { vertex_buffers_[frame_index_]->get_buffer() };
  VkDeviceSize offsets[] = { 0 };
  vkCmdBindVertexBuffers(cb, 0, 1, buffers, offsets);
  vkCmdBindIndexBuffer(cb, index_buffer_->get_buffer(), 0, VK_INDEX_TYPE_UINT32);

  frame_index_ = frame_index_ == 2 ? 0 : frame_index_ + 1;
};

} // namespace hnll::physics