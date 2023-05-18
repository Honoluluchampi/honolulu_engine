#pragma once
// hnll
#include <graphics/graphics_model.hpp>
#include <graphics/meshlet.hpp>

// std
#include <vector>

namespace hnll {
namespace graphics {

// forward declaration
class device;
class buffer;
class desc_pool;
class desc_layout;
struct vertex;
struct mesh_builder;

DEFINE_GRAPHICS_MODEL(static_meshlet, utils::shading_type::MESHLET)
{
  public:

    static_meshlet(device& device, std::vector<vertex>&& raw_vertices, std::vector<meshletBS>&& meshlets);

    static u_ptr<static_meshlet> create_from_file(device& device, std::string filename);

    void draw(VkCommandBuffer  command_buffer) const;

    // getter
    const buffer& get_vertex_buffer()  const;
    const buffer& get_meshlet_buffer() const;
    inline void* get_raw_vertices_data() { return raw_vertices_.data(); }
    inline void* get_meshlets_data()     { return meshlets_.data(); }
    inline uint32_t get_meshlets_count() { return meshlet_count_; }
    std::vector<VkDescriptorSetLayout> get_raw_desc_layouts() const;

    const std::vector<VkDescriptorSet>& get_desc_sets() const { return desc_sets_; }

    static std::vector<u_ptr<desc_layout>> default_desc_layouts(device& _device);

  private:
    void setup_descs();
    void create_desc_pool();
    void create_desc_buffers();
    void create_desc_set_layouts();
    void create_desc_sets();

    device& device_;
    std::vector<vertex>  raw_vertices_;
    std::vector<meshletBS> meshlets_;
    s_ptr<desc_pool>                desc_pool_;
    std::vector<u_ptr<buffer>>      desc_buffers_;
    std::vector<u_ptr<desc_layout>> desc_layouts_;
    std::vector<VkDescriptorSet>    desc_sets_;
    uint32_t meshlet_count_;
};

}} // namespace hnll::graphics