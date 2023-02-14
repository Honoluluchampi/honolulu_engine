#pragma once

// hnll
#include <utils/common_alias.hpp>

// lib
#include <vulkan/vulkan.h>

// std
#include <unordered_map>
#include <memory>
#include <string>
#include <functional>

// forward declaration
namespace hnll::geometry { class mesh_model; }

namespace hnll::graphics {

class device;
class buffer;
class mesh_builder;
class vertex;

class static_mesh
{
  public:
    // compatible with wavefront obj. file

    static_mesh(device& device, const mesh_builder &builder);
    ~static_mesh();

    static_mesh(const static_mesh &) = delete;
    static_mesh& operator=(const static_mesh &) = delete;

    static u_ptr<static_mesh> create_from_file(device &device, const std::string &filename);
    static u_ptr<static_mesh> create_from_geometry_mesh_model(device &device, const s_ptr<geometry::mesh_model>& gm);

    void bind(VkCommandBuffer command_buffer);
    void draw(VkCommandBuffer command_buffer);

    // setter
    bool has_index_buffer() const { return had_index_buffer_; }
    // getter
    const std::vector<vertex>& get_vertex_list() const;
    std::vector<vec3d>         get_vertex_position_list() const;
    unsigned                   get_face_count() const { return index_count_ / 3; }

  private:
    void create_vertex_buffers(const std::vector<vertex> &vertices);
    void create_index_buffers(const std::vector<uint32_t> &indices);

    device& device_;
    // contains buffer itself and buffer memory
    std::unique_ptr<buffer> vertex_buffer_;
    std::unique_ptr<buffer> index_buffer_;
    uint32_t vertex_count_;
    uint32_t index_count_;

    bool had_index_buffer_ = false;

    // for geometric process
    std::vector<vertex> vertex_list_{};
};

} // namespace hnll::graphics