#pragma once

// hnll
#include <utils/common_alias.hpp>
#include <graphics/desc_set.hpp>

namespace hnll::physics {

struct vertex
{
  alignas(16) vec3 position;
};

class mass_spring_cloth
{
  public:
    static s_ptr<mass_spring_cloth>create(int x_grid, int y_grid, float x_len, float y_len);
    explicit mass_spring_cloth(int x_grid, int y_grid, float x_len, float y_len);
    ~mass_spring_cloth();

    void bind(VkCommandBuffer cb);

    // getter
    inline uint32_t get_id() const { return cloth_id_; }
    inline int get_x_grid() const { return x_grid_; }
    inline int get_y_grid() const { return y_grid_; }
    inline uint32_t get_indices_count() const { return indices_count_; }

    static VkDescriptorSetLayout get_vk_desc_layout();
    std::vector<VkDescriptorSet> get_vk_desc_sets(int frame_index);

    // setter
    static void set_desc_layout();
    static void reset_desc_layout();

  private:
    std::vector<vertex> construct_mesh(float x_len, float y_len);
    std::vector<uint32_t> construct_index_buffer();
    void setup_desc_sets(std::vector<vertex>&& mesh, std::vector<uint32_t>&& indices);

    // ref to graphics_engine_core::device
    graphics::device& device_;
    uint32_t cloth_id_;

    u_ptr<graphics::desc_sets> desc_sets_;
    s_ptr<graphics::desc_pool> desc_pool_;

    graphics::buffer*       vertex_buffer_;
    u_ptr<graphics::buffer> index_buffer_;

    static u_ptr<graphics::desc_layout> desc_layout_;

    int x_grid_, y_grid_;
    uint32_t indices_count_;
};

} // namespace hnll::physics