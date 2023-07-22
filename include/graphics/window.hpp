#pragma once

// hnll
#include <utils/common_alias.hpp>

// lib
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

// std
#include <string>

namespace hnll::graphics {

class window
{
  public:
    window(const int w, const int h, const std::string name);
    window() {} // for singleton access
    ~window();

    // delete copy ctor, assignment (to prevent GLFWwindow* from double deleted)
    window(const window &) = delete;
    window& operator= (const window &) = delete;

    static u_ptr<window> create(const int w, const int h, const std::string& name)
    { return std::make_unique<window>(w, h, name); }

    void create_window_surface(VkInstance instance, VkSurfaceKHR* surface);

    // getter
    inline bool should_be_closed() { return glfwWindowShouldClose(glfw_window_); }
    inline bool is_resized() { return frame_buffer_resized_; }
    VkExtent2D  get_extent() { return { static_cast<uint32_t>(width_), static_cast<uint32_t>(height_)}; }
    GLFWwindow* get_glfw_window() const { return glfw_window_; }
    // setter
    void reset_window_resized_flag() { frame_buffer_resized_ = false; }

  private:
    static void frame_buffer_resize_callback(GLFWwindow *window, int width, int height);
    void init_window();

    int width_;
    int height_;
    bool frame_buffer_resized_ = false;

    std::string window_name_;
    GLFWwindow *glfw_window_;
};

} // namespace hnll::graphics