#pragma once

// hnll
#include <graphics/shader_binding_table.hpp>
#include <utils/common_alias.hpp>
#include <utils/frame_info.hpp>

namespace hnll {

#define DEFINE_RAY_TRACER(name, shading_type) class name : public game::ray_tracing_system<name, shading_type>

namespace game {

template<typename Derived, utils::shading_type type>
class ray_tracing_system
{
  public:
    static u_ptr<Derived> create() { return std::make_unique<Derived>(); }

    virtual ~ray_tracing_system() = default;

    // impl in each class
    void setup();
    void render(const utils::graphics_frame_info &frame_info);

    inline utils::shading_type get_shading_type() const { return shading_type_; }

    static void set_tlas_desc_set(VkDescriptorSet tlas_desc_set) { tlas_desc_set_ = tlas_desc_set; }

  private:
    ray_tracing_system() { shading_type_ = type; }

    void create_sbt(
      const std::vector<VkDescriptorSetLayout> &vk_desc_layouts,
      const std::vector<std::string> &shader_paths,
      const std::vector<VkShaderStageFlagBits> &shader_stages)
    { sbt_ = graphics::shader_binding_table::create(vk_desc_layouts, shader_paths, shader_stages); }

    void set_current_command(VkCommandBuffer command) { current_command_ = command; }

    inline void bind_pipeline() { vkCmdBindPipeline(current_command_, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, sbt_->get_pipeline()); }

    inline void bind_desc_sets(const std::vector<VkDescriptorSet>& vk_desc_sets)
    {
      vkCmdBindDescriptorSets(
        current_command_,
        VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        sbt_->get_pipeline_layout(),
        0,
        vk_desc_sets.size(),
        vk_desc_sets.data(),
        0,
        nullptr
      );
    }

    inline void dispatch_command(uint32_t width, uint32_t height, uint32_t depth)
    {
      vkCmdTraceRaysKHR(
        current_command_,
        sbt_->get_gen_region_p(), sbt_->get_miss_region_p(), sbt_->get_hit_region_p(), sbt_->get_callable_region_p(),
        width, height, depth
      );
    }

    u_ptr<graphics::shader_binding_table> sbt_;
    VkCommandBuffer current_command_ = nullptr;
    utils::shading_type shading_type_;

    static VkDescriptorSet tlas_desc_set_;

    friend Derived;
};

#define RT_API template <typename Derived, utils::shading_type type>
#define RT_TYPE ray_tracing_system<Derived, type>

RT_API VkDescriptorSet RT_TYPE::tlas_desc_set_ = nullptr;

};
} // namespace hnll::game