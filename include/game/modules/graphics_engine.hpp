#pragma once

// hnll
#include <game/shading_system.hpp>
#include <game/concepts.hpp>
#include <graphics/graphics_model_pool.hpp>
#include <utils/common_alias.hpp>
#include <utils/singleton.hpp>
#include <utils/frame_info.hpp>
#include <utils/vulkan_config.hpp>

#ifndef IMGUI_DISABLED
#include <game/modules/gui_engine.hpp>
#endif

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
  class desc_pool;
  class desc_sets;
  class buffer;
  class timeline_semaphore;
}

namespace game {

// should be wrapped by utils::singleton
class graphics_engine_core
{
  public:
    static constexpr int WIDTH = 960;
    static constexpr int HEIGHT = 820;
    static constexpr float MAX_FRAME_TIME = 0.05;

    graphics_engine_core(const std::string& window_name);
    ~graphics_engine_core();

    void wait_idle();

    // for graphics_engine::render()
    bool begin_frame();
    void record_default_render_command();
    VkCommandBuffer begin_command_buffer(int renderer_id);

    int  get_frame_index();
    VkDescriptorSet update_ubo(const utils::global_ubo& ubo, int frame_index);
    void begin_render_pass(VkCommandBuffer command_buffer, int renderer_id, VkExtent2D extent);
    void end_render_pass_and_frame(VkCommandBuffer command_buffer);
    void end_frame(VkCommandBuffer command_buffer);

    // getter
    template <graphics::GraphicsModel M>
    static M& get_graphics_model(const std::string& name)
    { return model_pool_->get_model<M>(name); }

    bool should_close_window() const;
    GLFWwindow* get_glfw_window() const ;
    graphics::renderer& get_renderer_r();
    graphics::timeline_semaphore& get_compute_semaphore_r();

    static VkDescriptorSetLayout get_global_desc_layout();
    static VkRenderPass get_default_render_pass() { return default_render_pass_; }
    static std::tuple<int, int> get_window_size();

  private:
    void init();
    void setup_ubo();
    void setup_global_shading_system_config();

    void cleanup();

    // construct as singleton in impl
    utils::single_ptr<graphics::window> window_;
    utils::single_ptr<graphics::device> device_;

    static u_ptr<graphics::renderer> renderer_;

    // global config for shading system
    static s_ptr<graphics::desc_pool>           global_pool_;
    static u_ptr<graphics::desc_sets>           global_desc_sets_;

    static VkRenderPass default_render_pass_;

    static u_ptr<graphics::graphics_model_pool> model_pool_;
};

template <ShadingSystem... S>
class graphics_engine
{
    using shading_system_map = std::unordered_map<uint32_t, std::variant<u_ptr<S>...>>;
  public:
    graphics_engine(const std::string &application_name);
    ~graphics_engine() { cleanup(); }

    static u_ptr<graphics_engine<S...>> create(const std::string &application_name)
    { return std::make_unique<graphics_engine<S...>>(application_name); }

    graphics_engine(const graphics_engine &) = delete;
    graphics_engine &operator= (const graphics_engine &) = delete;

    void render(const utils::game_frame_info& frame_info);

    template <ShadingSystem Head, ShadingSystem... Rest> void add_shading_system();
    void add_shading_system(){}

    template <ShadingSystem SS, RenderableComponent RC>
    void add_render_target(RC& rc)
    {
      auto key = static_cast<uint32_t>(rc.get_shading_type());
      std::get<u_ptr<SS>>(shading_systems_[key])->add_render_target(rc.get_rc_id(), rc);
    }

  private:
    void cleanup();

    utils::single_ptr<graphics::window> window_; // singleton

    utils::single_ptr<graphics_engine_core> core_;

    utils::rendering_type rendering_type_;
    static shading_system_map shading_systems_;
};

// impl
#define GRPH_ENGN_API  template <ShadingSystem... S>
#define GRPH_ENGN_TYPE graphics_engine<S...>

GRPH_ENGN_API typename graphics_engine<S...>::shading_system_map GRPH_ENGN_TYPE::shading_systems_;

GRPH_ENGN_API GRPH_ENGN_TYPE::graphics_engine(const std::string &application_name)
 : core_(utils::singleton<graphics_engine_core>::build_instance(application_name)),
   window_(utils::singleton<graphics::window>::build_instance())
{
  // construct all selected shading systems
  add_shading_system<S...>();
  rendering_type_ = utils::singleton<utils::vulkan_config>::get_single_ptr()->rendering;
}

GRPH_ENGN_API void GRPH_ENGN_TYPE::render(const utils::game_frame_info& frame_info)
{
  if (core_->begin_frame()) {
    int frame_index = core_->get_frame_index();

    // update
    utils::global_ubo ubo;
    ubo.projection   = frame_info.view.projection;
    ubo.view         = frame_info.view.view;
    ubo.inverse_view = frame_info.view.inverse_view;
    // temp
    ubo.point_lights[0] = {{0.f, -6.f, 0.f, 1.f}, { 1.f, 1.f, 1.f, 10.f}};
    ubo.lights_count = 1;
    ubo.ambient_light_color = { 1.f, 1.f, 1.f, 0.1f };

    VkCommandBuffer command_buffer;
    // setup command buffer which associated with proper render pass
    // draw whole window
    core_->record_default_render_command();

    // draw viewport
    command_buffer = core_->begin_command_buffer(1);
    auto window_extent = window_->get_extent();
    auto left   = gui_engine::get_left_window_ratio();
    auto bottom = gui_engine::get_bottom_window_ratio();

    if (rendering_type_ != utils::rendering_type::RAY_TRACING)  {
      core_->begin_render_pass(
        command_buffer,
        1,
        {static_cast<uint32_t>(window_extent.width * (1.f - left)),
         static_cast<uint32_t>(window_extent.height * (1.f - bottom))});
    }

    // TODO : no gui version

    utils::graphics_frame_info graphics_frame_info{
      frame_index,
      command_buffer,
      core_->update_ubo(ubo, frame_index),
      {}
    };

    for (auto& system_kv : shading_systems_) {
      std::visit([&graphics_frame_info](auto& system) { system->render(graphics_frame_info); }, system_kv.second);
    }

    if (rendering_type_ != utils::rendering_type::RAY_TRACING) {
      core_->end_render_pass_and_frame(command_buffer);
    }
    else
      core_->end_frame(command_buffer);
  }
}

GRPH_ENGN_API template <ShadingSystem Head, ShadingSystem... Rest>
void GRPH_ENGN_TYPE::add_shading_system()
{
  auto system = Head::create();
  system->setup();
  shading_systems_[static_cast<uint32_t>(system->get_shading_type())] = std::move(system);

  if constexpr (sizeof...(Rest) >= 1)
    add_shading_system<Rest...>();
}

GRPH_ENGN_API void GRPH_ENGN_TYPE::cleanup()
{
  for (auto& s : shading_systems_) {
    std::visit([](auto& system) { system.reset(); }, s.second);
  }
}
}} // namespace hnll::game