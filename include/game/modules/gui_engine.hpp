#pragma once

// hnll
#include <utils/common_alias.hpp>

// basic header
#include <imgui/imgui.h>

// api-specific  header
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

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

namespace graphics {
  class device;
  class window;
  class renderer;
}

namespace gui {
  class renderer;
}

namespace game {
class gui_engine {
  public:
    gui_engine(hnll::graphics::window &window, hnll::graphics::device &device);
    ~gui_engine();

    static u_ptr<gui_engine> create(graphics::window& window, graphics::device& device)
    { return std::make_unique<gui_engine>(window, device); }

    gui_engine(const gui_engine &) = delete;
    gui_engine &operator=(const gui_engine &) = delete;
    gui_engine(gui_engine &&) = default;
    gui_engine &operator=(gui_engine &&) = default;

    void begin_imgui();

    void render();

    void frame_render();

    const u_ptr<gui::renderer> &renderer_up() const { return renderer_up_; }

    graphics::renderer *renderer_p() const;

  private:
    // set up ImGui context
    void setup_imgui(hnll::graphics::device &device, GLFWwindow *window);

    // share the basic graphics object with hve, so there is nothing to do for now
    void setup_specific_vulkan_objects();

    void upload_font();

    void cleanup_vulkan();

    void create_descriptor_pool();

    static void glfw_error_callback(int error, const char *description) {
      fprintf(stderr, "Glfw Error %d: %s\n", error, description);
    }

    VkDevice device_;
    VkDescriptorPool descriptor_pool_;
    VkQueue graphics_queue_;

    ImGui_ImplVulkanH_Window main_window_data_;

    u_ptr<gui::renderer> renderer_up_;

    // TODO : make it consistent with hve
    int min_image_count_ = 2;
    bool swap_chain_rebuild_ = false;
    bool is_gui_engine_running_ = false;
};
}} // namespace hnll::game