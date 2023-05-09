// hnll
#include <graphics/graphics_models/static_meshlet.hpp>
#include <graphics/buffer.hpp>
#include <graphics/desc_set.hpp>
#include <graphics/utils.hpp>
#include <geometry/mesh_separation.hpp>
#include <geometry/he_mesh.hpp>
#include <utils/utils.hpp>

// std
#include <iostream>

namespace hnll::graphics {

static_meshlet::static_meshlet(device& device, std::vector<vertex> &&raw_vertices, std::vector<meshletBS> &&meshlets)
 : device_(device)
{
  raw_vertices_ = std::move(raw_vertices);
  meshlets_     = std::move(meshlets);
  meshlet_count_ = meshlets_.size();
  setup_descs();
}

u_ptr<static_meshlet> static_meshlet::create_from_file(hnll::graphics::device &device, std::string filename)
{
  std::vector<meshletBS> meshlets;

  auto filepath = utils::get_full_path(filename);

  // prepare required data
  auto mesh = geometry::he_mesh::create_from_obj_file(filepath);

  // if model's cache exists
  meshlets = geometry::mesh_separation::load_meshlet_cache(filename);
  if (meshlets.size() == 0) {
    meshlets = geometry::mesh_separation::separate_meshletBS(*mesh, filename);
  }

  return std::make_unique<static_meshlet>(device, mesh->move_raw_vertices(), std::move(meshlets));
}

void static_meshlet::draw(VkCommandBuffer _command_buffer) const
{
  // draw
  vkCmdDrawMeshTasksNV(
    _command_buffer,
    (meshlets_.size() + meshlet_constants::GROUP_SIZE - 1) / meshlet_constants::GROUP_SIZE,
    0
  );
}

void static_meshlet::setup_descs()
{
  create_desc_pool();
  create_desc_buffers();
  create_desc_set_layouts();
  create_desc_sets();
}

void static_meshlet::create_desc_pool()
{
  desc_pool_ = desc_pool::builder(device_)
    .set_max_sets(DESC_SET_COUNT)
    .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, DESC_SET_COUNT)
    .build();
}

void static_meshlet::create_desc_buffers()
{
  desc_buffers_.resize(DESC_SET_COUNT);

  desc_buffers_[VERTEX_DESC_ID] = graphics::buffer::create_with_staging(
    device_,
    sizeof(vertex),
    raw_vertices_.size(),
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    raw_vertices_.data()
  );

  desc_buffers_[MESHLET_DESC_ID] = graphics::buffer::create_with_staging(
    device_,
    sizeof(meshletBS),
    meshlets_.size(),
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    meshlets_.data()
  );
}

void static_meshlet::create_desc_set_layouts()
{
  desc_layouts_.resize(DESC_SET_COUNT);

  for (int i = 0; i < DESC_SET_COUNT; i++) {
    desc_layouts_[i] = desc_layout::builder(device_)
      .add_binding(
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        VK_SHADER_STAGE_TASK_BIT_NV | VK_SHADER_STAGE_MESH_BIT_NV)
      .build();
  }
}

void static_meshlet::create_desc_sets()
{
  desc_sets_.resize(DESC_SET_COUNT);

  for (int i = 0; i < DESC_SET_COUNT; i++) {
    auto buffer_info = desc_buffers_[i]->desc_info();
    desc_writer(*desc_layouts_[i], *desc_pool_)
      .write_buffer(0, &buffer_info)
      .build(desc_sets_[i]);
  }
}

// getter
const buffer& static_meshlet::get_vertex_buffer()  const
{ return *desc_buffers_[VERTEX_DESC_ID]; }

const buffer& static_meshlet::get_meshlet_buffer() const
{ return *desc_buffers_[MESHLET_DESC_ID]; }

std::vector<VkDescriptorSetLayout> static_meshlet::get_raw_desc_layouts() const
{
  std::vector<VkDescriptorSetLayout> ret;
  for (int i = 0; i < DESC_SET_COUNT; i++) {
    ret.push_back(desc_layouts_[i]->get_descriptor_set_layout());
  }
  return ret;
}

std::vector<u_ptr<desc_layout>> static_meshlet::default_desc_layouts(device& device)
{
  std::vector<u_ptr<desc_layout>> ret;

  ret.resize(DESC_SET_COUNT);

  for (int i = 0; i < DESC_SET_COUNT; i++) {
    ret[i] = desc_layout::builder(device)
      .add_binding(
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        VK_SHADER_STAGE_TASK_BIT_NV | VK_SHADER_STAGE_MESH_BIT_NV)
      .build();
  }

  return ret;
}

} // namespace hnll::graphics