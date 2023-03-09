#pragma once

// hnll
#include <graphics/graphics_model.hpp>

// forward declaration
namespace hnll::geometry { class mesh_model; }

namespace hnll::graphics {

class device;
class buffer;
class obj_loader;
class vertex;
class texture_image;

using static_mesh = graphics_model<utils::shading_type::MESH>;

template<>
class graphics_model<utils::shading_type::MESH> {
  public:
    // compatible with wavefront obj. file

    graphics_model(device& _device, const obj_loader &builder);

    static u_ptr<static_mesh> create_from_file(device &device, const std::string &filename);
    static u_ptr<static_mesh> create_from_geometry_mesh_model(device &device, const s_ptr<geometry::mesh_model> &gm);

    void bind(VkCommandBuffer command_buffer);
    void draw(VkCommandBuffer command_buffer);

    // setter
    bool has_index_buffer() const { return had_index_buffer_; }

    // getter
    const std::vector<vertex> &get_vertex_list() const;
    std::vector<vec3d> get_vertex_position_list() const;

    unsigned get_face_count() const { return index_count_ / 3; }

  private:
    void create_vertex_buffers(const std::vector<vertex> &vertices);
    void create_index_buffers(const std::vector<uint32_t> &indices);

    device& device_;
    // contains buffer itself and buffer memory
    u_ptr<buffer> vertex_buffer_;
    u_ptr<buffer> index_buffer_;

    uint32_t vertex_count_;
    uint32_t index_count_;

    bool had_index_buffer_ = false;

    // texture
    u_ptr<texture_image> texture_;
    bool is_textured_ = false;

    // for geometric process
    std::vector<vertex> vertex_list_{};
};

}