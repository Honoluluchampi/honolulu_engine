// hnll
#include <graphics/graphics_models/static_mesh.hpp>
#include <graphics/device.hpp>
#include <graphics/buffer.hpp>
#include <graphics/utils.hpp>
#include <utils/utils.hpp>

// std
#include <iostream>
#include <unordered_map>

namespace hnll::graphics {

static_mesh::graphics_model(device& device, const obj_loader &loader) : device_{device}
{
  create_vertex_buffers(loader.vertices);
  create_index_buffers(loader.indices);

  vertex_list_ = std::move(loader.vertices);
}

u_ptr<static_mesh> static_mesh::create_from_file(device &device, const std::string &filename)
{
  obj_loader builder;
  builder.load_model(filename);
  std::cout << filename << " vertex count: " << builder.vertices.size() << "\n";
  return std::make_unique<static_mesh>(device, builder);
}

void static_mesh::create_vertex_buffers(const std::vector<vertex> &vertices)
{
  // vertexCount must be larger than 3 (triangle)
  // use a host visible buffer as temporary buffer, use a device local buffer as actual vertex buffer
  vertex_count_ = static_cast<uint32_t>(vertices.size());
  VkDeviceSize buffer_size = sizeof(vertices[0]) * vertex_count_;
  uint32_t vertex_size = sizeof(vertices[0]);

  // staging buffer creation
  buffer staging_buffer {
    device_,
    vertex_size, // for calculating alignment
    vertex_count_, // same as above
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // usage
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT // property
  };
  // mapping the data to the buffer
  staging_buffer.map();
  staging_buffer.write_to_buffer((void *)vertices.data());

  // vertex buffer creation
  vertex_buffer_ = std::make_unique<buffer>(
    device_,
    vertex_size, // for calculating alignment
    vertex_count_, // same as above
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, // usage
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT// property
  );
  // copy the data from staging buffer to the vertex buffer
  device_.copy_buffer(staging_buffer.get_buffer(), vertex_buffer_->get_buffer(), buffer_size);
  // staging buffer is automatically freed in the dtor
}

void static_mesh::create_index_buffers(const std::vector<uint32_t> &indices)
{
  index_count_ = static_cast<uint32_t>(indices.size());
  // if there is no index, nothing to do
  had_index_buffer_ = index_count_ > 0;
  if (!had_index_buffer_) return;

  VkDeviceSize buffer_size = sizeof(indices[0]) * index_count_;
  uint32_t indexSize = sizeof(indices[0]);

  // copy the data to the staging buffer
  buffer staging_buffer {
    device_,
    indexSize,
    index_count_,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
  };

  staging_buffer.map();
  staging_buffer.write_to_buffer((void *)indices.data());

  index_buffer_ = std::make_unique<buffer> (
    device_,
    indexSize,
    index_count_,
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT // optimal type of the memory type
  );

  // copy the data from staging buffer to the vertex buffer
  device_.copy_buffer(staging_buffer.get_buffer(), index_buffer_->get_buffer(), buffer_size);
}

void static_mesh::bind(VkCommandBuffer command_buffer)
{
  VkBuffer buffers[] = {vertex_buffer_->get_buffer()};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(command_buffer, 0, 1, buffers, offsets);

  // last parameter should be same as the type of the Build::indices
  if (had_index_buffer_)
    vkCmdBindIndexBuffer(command_buffer, index_buffer_->get_buffer(), 0, VK_INDEX_TYPE_UINT32);
}

void static_mesh::draw(VkCommandBuffer command_buffer)
{
  if (had_index_buffer_)
    vkCmdDrawIndexed(command_buffer, index_count_, 1, 0, 0, 0);
  else
    vkCmdDraw(command_buffer, vertex_count_, 1, 0, 0);
}

const std::vector<vertex>& static_mesh::get_vertex_list() const { return vertex_list_; }

std::vector<vec3d> static_mesh::get_vertex_position_list() const
{
  // extract position data from vertex_list_
  std::vector<vec3d> vertex_position_list;
  for (const auto &vertex : vertex_list_) {
    vertex_position_list.emplace_back(vertex.position.cast<double>());
  }
  return vertex_position_list;
}

} // namespace hnll::graphics