// hnll
#include <game/engine.hpp>
#include <game/modules/gui_engine.hpp>
#include <graphics/renderer.hpp>
#include <utils/rendering_utils.hpp>

// std
#include <iostream>

namespace hnll::game {

// static members
u_ptr<gui_engine> engine_core::gui_engine_;
std::vector<u_ptr<std::function<void(GLFWwindow *, int, int, int)>>> engine_core::glfw_mouse_button_callbacks_;
utils::viewer_info engine_core::viewer_info_;

engine_core::engine_core(const std::string &application_name, utils::rendering_type rendering_type)
 : graphics_engine_core_(utils::singleton<graphics_engine_core>::get_instance(application_name, rendering_type))
{
#ifndef IMGUI_DISABLED
  gui_engine_ = gui_engine::create(graphics_engine_core_.get_window_r(), graphics_engine_core_.get_device_r());
  // configure dependency between renderers
  graphics_engine_core_.get_renderer_r().set_next_renderer(gui_engine_->renderer_p());
#endif

  old_time_ = std::chrono::system_clock::now();;
  // glfw
  set_glfw_mouse_button_callbacks();
}

engine_core::~engine_core() { cleanup(); }

void engine_core::render_gui()
{
  if (!hnll::graphics::renderer::swap_chain_recreated_) {
    gui_engine_->begin_imgui();
    update_gui();
    gui_engine_->render();
  }
}

void engine_core::cleanup() { gui_engine_.reset(); }

// glfw
void engine_core::set_glfw_mouse_button_callbacks()
{ glfwSetMouseButtonCallback(graphics_engine_core_.get_glfw_window(), glfw_mouse_button_callback); }

void engine_core::add_glfw_mouse_button_callback(u_ptr<std::function<void(GLFWwindow* window, int button, int action, int mods)>>&& func)
{
  glfw_mouse_button_callbacks_.emplace_back(std::move(func));
  set_glfw_mouse_button_callbacks();
}

void engine_core::glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
  for (const auto& func : glfw_mouse_button_callbacks_)
    func->operator()(window, button, action, mods);

#ifndef IMGUI_DISABLED
  ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
#endif
}

} // namespace hnll::game