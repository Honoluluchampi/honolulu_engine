// hnll
#include <graphics/window.hpp>

// std
#include <stdexcept>

namespace hnll::graphics {

window::window(const int w, const int h, const std::string name) : width_(w), height_(h), window_name_(name)
{
  init_window();
}

window::~window()
{
  glfwDestroyWindow(glfw_window_);
  glfwTerminate();
}

void window::init_window()
{
  glfwInit();
  // disable openGL
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  glfw_window_ = glfwCreateWindow(width_, height_, window_name_.c_str(), nullptr, nullptr);
  // handle framebuffer resizes
  glfwSetWindowUserPointer(glfw_window_, this);
  glfwSetFramebufferSizeCallback(glfw_window_, frame_buffer_resize_callback);
}

void window::create_window_surface(VkInstance instance, VkSurfaceKHR* surface)
{
  // glfwCreateWindowSurface is implemented to multi-platfowm
  if (glfwCreateWindowSurface(instance, glfw_window_, nullptr, surface) != VK_SUCCESS)
    throw std::runtime_error("failed to create window surface");
}

void window::frame_buffer_resize_callback(GLFWwindow *window, int width, int height)
{
  auto graphics_window = reinterpret_cast<hnll::graphics::window *>(glfwGetWindowUserPointer(window));
  graphics_window->frame_buffer_resized_ = true;
  graphics_window->width_ = width;
  graphics_window->height_ = height;
}

} // namespace hnll::graphics