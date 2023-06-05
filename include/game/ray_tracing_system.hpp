#pragma once

// hnll
#include <graphics/shader_binding_table.hpp>
#include <utils/common_alias.hpp>
#include <utils/frame_info.hpp>

namespace hnll {

namespace game {

template<typename Derived>
class ray_tracing_system
{
  public:
    static u_ptr<ray_tracing_system> create(
      graphics::device &device);

    // impl in each class
    void render(const utils::graphics_frame_info &frame_info);

  private:
    ray_tracing_system(graphics::device& device) : device_(device)
    { static_cast<Derived*>(this)->setup(); }

    // impl in each class
    void setup();

    void create_sbt(
      const std::vector<VkDescriptorSetLayout> &vk_desc_layouts,
      const std::vector<std::string> &shader_paths,
      const std::vector<VkShaderStageFlagBits> &shader_stages)
    { sbt_ = graphics::shader_binding_table::create(device_, vk_desc_layouts, shader_paths, shader_stages); }

    graphics::device& device_;
    u_ptr<graphics::shader_binding_table> sbt_;

    friend Derived;
};

#define RT_API template <typename Derived>
#define RT_TYPE ray_tracing_system<Derived>

};
} // namespace hnll::game