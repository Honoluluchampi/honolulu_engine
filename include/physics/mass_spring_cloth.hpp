#pragma once

// hnll
#include <utils/common_alias.hpp>
#include <graphics/desc_set.hpp>

namespace hnll::physics {

struct vertex
{
  alignas(16) vec3 position;
  alignas(16) vec3 normal;
};

class mass_spring_cloth
{
  public:
    static s_ptr<mass_spring_cloth>create();
    mass_spring_cloth();
    ~mass_spring_cloth();

    // getter
    inline uint32_t get_id() const { return cloth_id_; }

    static VkDescriptorSetLayout get_vk_desc_layout();
    std::vector<VkDescriptorSet> get_vk_desc_sets(int frame_index);

    // setter
    static void set_desc_layout();
    static void reset_desc_layout();

  private:
    void setup_desc_sets();

    // ref to graphics_engine_core::device
    graphics::device& device_;
    uint32_t cloth_id_;

    u_ptr<graphics::desc_sets> desc_sets_;
    s_ptr<graphics::desc_pool> desc_pool_;

    static u_ptr<graphics::desc_layout> desc_layout_;
};

} // namespace hnll::physics