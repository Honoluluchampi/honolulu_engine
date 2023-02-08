// hnll
#include <game/engine.hpp>
#include <game/modules/gui_engine.hpp>
#include <graphics/renderer.hpp>
#include <utils/rendering_utils.hpp>

// std
#include <iostream>

namespace hnll::game {

// static members
u_ptr<graphics_engine> engine::graphics_engine_;
std::vector<u_ptr<std::function<void(GLFWwindow *, int, int, int)>>> engine::glfw_mouse_button_callbacks_;

engine::engine(const std::string& application_name, utils::rendering_type rendering_type)
{
  graphics_engine_ = graphics_engine::create(application_name, rendering_type);

#ifndef IMGUI_DISABLED
  gui_engine_ = gui_engine::create(graphics_engine_->get_window_r(), graphics_engine_->get_device_r());
  // configure dependency between renderers
  graphics_engine_->get_renderer_r().set_next_renderer(gui_engine_->renderer_p());
#endif

  // glfw
  set_glfw_mouse_button_callbacks();
}

engine::~engine() {}

void engine::run()
{
  while (!graphics_engine_->should_close_window()) {
    update();
    render();
  }
}

void engine::update()
{

}

void engine::render()
{
  graphics_engine_->render();
#ifndef IMGUI_DISABLED
  if (!hnll::graphics::renderer::swap_chain_recreated_){
    gui_engine_->begin_imgui();
    // update_gui();
    gui_engine_->render();
  }
#endif
}

// glfw
void engine::set_glfw_mouse_button_callbacks()
{
  glfwSetMouseButtonCallback(graphics_engine_->get_glfw_window(), glfw_mouse_button_callback);
}

void engine::add_glfw_mouse_button_callback(u_ptr<std::function<void(GLFWwindow* window, int button, int action, int mods)>>&& func)
{
  glfw_mouse_button_callbacks_.emplace_back(std::move(func));
  set_glfw_mouse_button_callbacks();
}

void engine::glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
  for (const auto& func : glfw_mouse_button_callbacks_)
    func->operator()(window, button, action, mods);

#ifndef IMGUI_DISABLED
  ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
#endif
}

} // namespace hnll::game