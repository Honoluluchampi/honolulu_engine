#pragma once

// hnll
#include <game/shading_system.hpp>
#include <game/concepts.hpp>
#include <utils/common_alias.hpp>
#include <utils/singleton.hpp>

// std
#include <map>
#include <vector>
#include <variant>

// lib
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

namespace hnll {

namespace graphics {
  class window;
  class device;
  class renderer;
  class swap_chain;
  class desc_layout;
  class desc_pool;
  class buffer;
}

namespace utils {
  enum class rendering_type;
}

namespace game {

// should be wrapped by utils::singleton
class graphics_engine_core
{
  public:
    static constexpr int WIDTH = 960;
    static constexpr int HEIGHT = 820;
    static constexpr float MAX_FRAME_TIME = 0.05;

    graphics_engine_core(const std::string& window_name, utils::rendering_type rendering_type);
    ~graphics_engine_core();

    void wait_idle();

    // for graphics_engine::render()
    VkCommandBuffer begin_frame();
    int  get_frame_index();
    VkDescriptorSet update_ubo(const utils::global_ubo& ubo, int frame_index);
    void begin_swap_chain_render_pass(VkCommandBuffer command_buffer);
    void end_swap_chain_and_frame(VkCommandBuffer command_buffer);

    // getter
    bool should_close_window() const;
    GLFWwindow* get_glfw_window() const ;
    graphics::window& get_window_r();
    graphics::device& get_device_r();
    graphics::renderer& get_renderer_r();

    static VkDescriptorSetLayout get_global_desc_layout() { return vk_global_desc_layout_; }
    static VkRenderPass get_default_render_pass() { return default_render_pass_; }

  private:
    void init();
    void setup_ubo();
    void setup_global_shading_system_config();

    void cleanup();

    // construct in impl
    static u_ptr<graphics::window> window_;
    static u_ptr<graphics::device> device_;
    static u_ptr<graphics::renderer> renderer_;

    static u_ptr<graphics::desc_layout>  global_set_layout_;
    u_ptr<graphics::desc_pool>           global_pool_;
    std::vector<u_ptr<graphics::buffer>> ubo_buffers_;
    std::vector<VkDescriptorSet>         global_desc_sets_;

    // global config for shading system
    static VkDescriptorSetLayout vk_global_desc_layout_;
    static VkRenderPass default_render_pass_;
};

template <ShadingSystem... S>
class graphics_engine
{
    using shading_system_map = std::map<uint32_t, std::variant<u_ptr<S>...>>;
  public:
    static constexpr int WIDTH = 960;
    static constexpr int HEIGHT = 820;
    static constexpr float MAX_FRAME_TIME = 0.05;

    graphics_engine(const std::string &application_name, utils::rendering_type rendering_type);
    ~graphics_engine() { cleanup(); }

    static u_ptr<graphics_engine<S...>> create(const std::string &application_name, utils::rendering_type rendering_type)
    { return std::make_unique<graphics_engine<S...>>(application_name, rendering_type); }

    graphics_engine(const graphics_engine &) = delete;
    graphics_engine &operator= (const graphics_engine &) = delete;

    void render(const utils::viewer_info& viewer_info);

    template <ShadingSystem SS> void add_shading_system();

  private:
    void cleanup();

    graphics_engine_core& core_;
    static shading_system_map shading_systems_;
};

// impl
#define GRPH_ENGN_API  template <ShadingSystem... S>
#define GRPH_ENGN_TYPE graphics_engine<S...>

GRPH_ENGN_API typename graphics_engine<S...>::shading_system_map GRPH_ENGN_TYPE::shading_systems_;

GRPH_ENGN_API GRPH_ENGN_TYPE::graphics_engine(const std::string &application_name, utils::rendering_type rendering_type)
 : core_(utils::singleton<graphics_engine_core>::get_instance(application_name, rendering_type))
{
  // construct all selected shading systems
  add_shading_system<S...>();
}

GRPH_ENGN_API void GRPH_ENGN_TYPE::render(const utils::viewer_info& viewer_info)
{
  if (auto command_buffer = core_.begin_frame()) {
    int frame_index = core_.get_frame_index();

    // update
    utils::global_ubo ubo;
    ubo.projection   = viewer_info.projection;
    ubo.view         = viewer_info.view;
    ubo.inverse_view = viewer_info.inverse_view;
    // temp
    ubo.point_lights[0] = {{0.f, -6.f, 0.f, 0.f}, { 1.f, 1.f, 1.f, 1.f}};
    ubo.lights_count = 1;
    ubo.ambient_light_color = { 0.6f, 0.6f, 0.6f, 0.6f };

    utils::frame_info frame_info{
      frame_index,
      command_buffer,
      core_.update_ubo(ubo, frame_index),
      {}
    };

    core_.begin_swap_chain_render_pass(command_buffer);

    for (auto& system_kv : shading_systems_) {
      auto& system = *system_kv.second;
    }

    core_.end_swap_chain_and_frame(command_buffer);
  }
}

GRPH_ENGN_API template <ShadingSystem SS> void GRPH_ENGN_TYPE::add_shading_system()
{
  auto system = SS::create(core_.get_device_r());
  shading_systems_[static_cast<uint32_t>(system->get_shading_type())] = std::move(system);
}

GRPH_ENGN_API void GRPH_ENGN_TYPE::cleanup()
{
  for (auto& s : shading_systems_) {
    std::visit([](auto& system) { system.reset(); }, s.second);
  }
}
}} // namespace hnll::game