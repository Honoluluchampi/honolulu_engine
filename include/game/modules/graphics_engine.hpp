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

    static u_ptr<graphics_engine_core> create(const std::string& window_name, utils::rendering_type rendering_type);


    void wait_idle();

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

    // construct in impl
    static u_ptr<graphics::window> window_;
    static u_ptr<graphics::device> device_;
    static u_ptr<graphics::renderer> renderer_;

    static u_ptr<graphics::desc_layout> global_set_layout_;

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

    graphics_engine(const std::string& window_name, utils::rendering_type rendering_type);
    ~graphics_engine();

    static u_ptr<graphics_engine> create(const std::string& window_name, utils::rendering_type rendering_type);

    graphics_engine(const graphics_engine &) = delete;
    graphics_engine &operator= (const graphics_engine &) = delete;

    void render();

    void wait_idle();

    template <ShadingSystem SS> void add_shading_system(u_ptr<SS>&& system)
    { shading_systems_[static_cast<uint32_t>(system->get_shading_type())] = std::move(system); }

    // getter
    bool should_close_window() const;
    GLFWwindow* get_glfw_window() const ;
    graphics::window& get_window_r();
    graphics::device& get_device_r();
    graphics::renderer& get_renderer_r();

  private:
    void init();
    void setup_ubo();
    void setup_shading_system_config();
    void setup_default_shading_systems();

    graphics_engine_core& core_;
    static shading_system_map shading_systems_;
};

}
} // namespace hnll::graphics
