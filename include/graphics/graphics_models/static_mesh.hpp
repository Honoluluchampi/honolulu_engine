#pragma once

// hnll
#include <graphics/graphics_model.hpp>

// forward declaration
namespace hnll::geometry { class he_mesh; }

namespace hnll::graphics {

class device;
class buffer;
class obj_loader;
class vertex;

DEFINE_GRAPHICS_MODEL(static_mesh, utils::shading_type::MESH) {
  public:
    static_mesh(
      device& _device,
      const obj_loader &builder,
      bool for_ray_tracing);

    static u_ptr<static_mesh> create_from_file(
      device &device,
      const std::string &filename,
      bool for_ray_tracing = false);

    static u_ptr<static_mesh> create_from_geometry_mesh_model(device &device, const s_ptr<geometry::he_mesh> &gm);

    void bind(VkCommandBuffer command_buffer);
    void draw(VkCommandBuffer command_buffer) const;

    // setter
    bool has_index_buffer() const { return had_index_buffer_; }

    // getter
    const std::vector<vertex> &get_vertex_list() const;
    std::vector<vec3d> get_vertex_position_list() const;

    VkBuffer get_vertex_vk_buffer() const;
    VkBuffer get_index_vk_buffer() const;
    VkDescriptorBufferInfo get_vertex_buffer_info() const;
    VkDescriptorBufferInfo get_index_buffer_info() const;

    unsigned get_vertex_count() const { return vertex_count_; }
    unsigned get_face_count()   const { return index_count_ / 3; }

  private:
    void create_vertex_buffers(const std::vector<vertex> &vertices, bool for_ray_tracing);
    void create_index_buffers(const std::vector<uint32_t> &indices, bool for_ray_tracing);

    device& device_;
    // contains buffer itself and buffer memory
    u_ptr<buffer> vertex_buffer_;
    u_ptr<buffer> index_buffer_;

    uint32_t vertex_count_;
    uint32_t index_count_;

    bool had_index_buffer_ = false;

    // for geometric process
    std::vector<vertex> vertex_list_{};
};

}