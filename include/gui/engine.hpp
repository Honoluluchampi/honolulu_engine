#pragma once

// hnll
#include <graphics/engine.hpp>
#include <gui/renderer.hpp>

// basic header
#include <imgui.h>

// api-specific  header
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

// lib
#include <GLFW/glfw3.h>

// std
#include <stdio.h>          // printf, fprintf
#include <stdlib.h>         // abort

// debug
#ifndef NDEBUG
#define IMGUI_VULKAN_DEBUG_REPORT
#endif

namespace hnll {
namespace gui {

class engine
{
public:
  engine(hnll::graphics::window& window, hnll::graphics::device& device);
  ~engine();
  engine(const engine&) = delete;
  engine& operator=(const engine&) = delete;
  engine(engine&&) = default;
  engine& operator=(engine&&) = default;

  void begin_imgui();
  void render();
  void frame_render();
  void update(Eigen::Vector3d& translation);

  const u_ptr<renderer>& renderer_up() const { return renderer_up_; }
  gui::renderer* renderer_p() const { return renderer_up_.get(); }
  
private:
  // set up ImGui context
  void setup_imgui(hnll::graphics::device& device, GLFWwindow* window);
  // share the basic graphics object with hve, so there is nothing to do for now
  void setup_specific_vulkan_objects();
  void upload_font();
  void cleanup_vulkan();
  void create_descriptor_pool();

  static void glfw_error_callback(int error, const char* description)
  { fprintf(stderr, "Glfw Error %d: %s\n", error, description); }

  VkDevice device_;
  VkDescriptorPool descriptor_pool_;
  VkQueue graphics_queue_;

  ImGui_ImplVulkanH_Window main_window_data_;

  u_ptr<renderer> renderer_up_;

  // TODO : make it consistent with hve
  int min_image_count_ = 2;
  bool swap_chain_rebuild_ = false;
  bool is_gui_engine_running_ = false;

  // temp
  float vec_[3] = {0, 0, 0};
};

} // namespace gui
} // namespace hnll 