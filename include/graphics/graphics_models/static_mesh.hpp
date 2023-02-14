#pragma once

// hnll
#include <graphics/graphics_model.hpp>

// forward declaration
namespace hnll::geometry { class mesh_model; }

namespace hnll::graphics {

class buffer;
class mesh_builder;
class vertex;

using static_mesh = graphics_model<utils::shading_type::MESH>;

template<>
class graphics_model<utils::shading_type::MESH> {
  public:
    // compatible with wavefront obj. file

    graphics_model<utils::shading_type::MESH>(device &device, const mesh_builder &builder);
    ~graphics_model<utils::shading_type::MESH>();

    graphics_model<utils::shading_type::MESH>(const graphics_model<utils::shading_type::MESH> &) = delete;
    graphics_model<utils::shading_type::MESH> &operator=(const graphics_model<utils::shading_type::MESH> &) = delete;

    static u_ptr<graphics_model<utils::shading_type::MESH>> create_from_file(device &device, const std::string &filename);
    static u_ptr<graphics_model<utils::shading_type::MESH>> create_from_geometry_mesh_model(device &device, const s_ptr<geometry::mesh_model> &gm);

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

    // contains buffer itself and buffer memory
    std::unique_ptr<buffer> vertex_buffer_;
    std::unique_ptr<buffer> index_buffer_;
    uint32_t vertex_count_;
    uint32_t index_count_;

    bool had_index_buffer_ = false;

    // for geometric process
    std::vector<vertex> vertex_list_{};
};

}