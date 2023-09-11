// hnll
#include <game/engine.hpp>
#include <game/modules/gui_engine.hpp>
#include <graphics/renderer.hpp>

// std
#include <iostream>

namespace hnll::game {

// static members
std::vector<std::function<void(GLFWwindow *, int, int, int)>> engine_core::glfw_mouse_button_callbacks_;
utils::viewer_info engine_core::viewer_info_;

engine_core::engine_core(const std::string &application_name, utils::rendering_type rendering_type)
 : graphics_engine_core_(utils::singleton<graphics_engine_core>::build_instance(application_name, rendering_type)),
#ifndef IMGUI_DISABLED
   gui_engine_(utils::singleton<gui_engine>::build_instance(rendering_type))
#endif
{
#ifndef IMGUI_DISABLED
  // configure dependency between renderers
  graphics_engine_core_->get_renderer_r().set_next_renderer(gui_engine_->renderer_p());
#endif

  old_time_ = std::chrono::system_clock::now();;
  // glfw
  set_glfw_callbacks();
}

engine_core::~engine_core() { cleanup(); }

void engine_core::begin_imgui()
{ gui_engine_->begin_imgui(); }

void engine_core::render_gui()
{
  if (!hnll::graphics::renderer::swap_chain_recreated_) {
    gui_engine_->render();
  }
}

void engine_core::cleanup() {  }

float engine_core::get_dt()
{
  std::chrono::system_clock::time_point new_time = std::chrono::system_clock::now();
  while (std::chrono::duration<float, std::chrono::seconds::period>(new_time - old_time_).count() < 1.f / max_fps_) {
    new_time = std::chrono::system_clock::now();
  }
  float dt = std::chrono::duration<float, std::chrono::seconds::period>(new_time - old_time_).count();
  old_time_ = new_time;
  return dt;
}

// glfw
void engine_core::set_glfw_callbacks()
{
  auto* window = graphics_engine_core_->get_glfw_window();
  glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
//  glfwSetCursorPosCallback(window, glfw_cursor_pos_callback);
}

vec2 engine_core::get_cursor_pos() const
{
  double xpos, ypos;
  glfwGetCursorPos(graphics_engine_core_->get_glfw_window(), &xpos, &ypos);
  auto left = gui_engine::get_left_window_ratio();
  auto window_size = graphics_engine_core::get_window_size();

  return { xpos - left * std::get<0>(window_size), ypos };
}

void engine_core::add_glfw_mouse_button_callback(std::function<void(GLFWwindow* window, int button, int action, int mods)>&& func)
{ glfw_mouse_button_callbacks_.emplace_back(std::move(func)); }

void engine_core::glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
  for (const auto& func : glfw_mouse_button_callbacks_)
    func(window, button, action, mods);

#ifndef IMGUI_DISABLED
  ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
#endif
}

} // namespace hnll::game