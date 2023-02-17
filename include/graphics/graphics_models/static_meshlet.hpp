#pragma once
// hnll
#include <graphics/graphics_model.hpp>
#include <graphics/meshlet.hpp>

namespace hnll {
namespace graphics {

// forward declaration
class device;
class buffer;
class descriptor_pool;
class descriptor_set_layout;
struct frame_info;
struct vertex;
struct mesh_builder;

using static_meshlet = graphics_model<utils::shading_type::MESHLET>;

template<>
class graphics_model<utils::shading_type::MESHLET>
{
  public:

    graphics_model(std::vector<vertex>&& raw_vertices, std::vector<meshlet>&& meshlets);

    static u_ptr<static_meshlet> create_from_file(device& _device, std::string _filename);

    void bind(
      VkCommandBuffer               _command_buffer,
      std::vector<VkDescriptorSet>  _external_desc_set,
      VkPipelineLayout              _pipeline_layout);
    void draw(VkCommandBuffer  _command_buffer);

    // getter
    const buffer& get_vertex_buffer()  const;
    const buffer& get_meshlet_buffer() const;
    inline void* get_raw_vertices_data() { return raw_vertices_.data(); }
    inline void* get_meshlets_data()     { return meshlets_.data(); }
    inline uint32_t get_meshlets_count() { return meshlet_count_; }
    std::vector<VkDescriptorSetLayout> get_raw_desc_set_layouts() const;

    static std::vector<u_ptr<descriptor_set_layout>> default_desc_set_layouts(device& _device);

  private:
    void setup_descs(device& _device);
    void create_desc_pool(device& _device);
    void create_desc_buffers(device& _device);
    void create_desc_set_layouts(device& _device);
    void create_desc_sets();

    std::vector<vertex>  raw_vertices_;
    std::vector<meshlet> meshlets_;
    u_ptr<descriptor_pool>                    desc_pool_;
    std::vector<u_ptr<buffer>>                desc_buffers_;
    std::vector<u_ptr<descriptor_set_layout>> desc_set_layouts_;
    std::vector<VkDescriptorSet>              desc_sets_;
    uint32_t meshlet_count_;
};

}} // namespace hnll::graphics