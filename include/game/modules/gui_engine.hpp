#pragma once

// hnll
#include <utils/common_alias.hpp>
#include <utils/singleton.hpp>

// lib
#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

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

namespace utils {
  enum class rendering_type;
  template <typename T> struct single_ptr;
}

namespace game {
class gui_engine {
  public:
    gui_engine();
    ~gui_engine();

    static u_ptr<gui_engine> create();

    gui_engine(const gui_engine &) = delete;
    gui_engine &operator=(const gui_engine &) = delete;
    gui_engine(gui_engine &&) = default;
    gui_engine &operator=(gui_engine &&) = default;

    void begin_imgui();

    void render();

    void frame_render();

    // getter
    const u_ptr<gui::renderer> &renderer_up() const { return renderer_up_; }
    graphics::renderer *renderer_p() const;

    static float get_left_window_ratio();
    static float get_bottom_window_ratio();
    static ImVec2 get_viewport_size() { return viewport_size_; };

    VkDescriptorSetLayout get_vp_image_desc_layout();
    std::vector<VkDescriptorSet> get_vp_image_desc_sets();
    void transition_vp_image_layout(int frame_index, VkImageLayout new_layout, VkCommandBuffer command);

  private:
    // set up ImGui context
    void setup_imgui(hnll::graphics::device &device, GLFWwindow *window);

    // share the basic graphics object with hve, so there is nothing to do for now
    void setup_specific_vulkan_objects();
    void setup_viewport();

    void upload_font();

    void cleanup_vulkan();

    void create_descriptor_pool();

    static void glfw_error_callback(int error, const char *description) {
      fprintf(stderr, "Glfw Error %d: %s\n", error, description);
    }

    utils::single_ptr<graphics::device> device_;
    VkDescriptorPool descriptor_pool_;
    VkQueue graphics_queue_;

    ImGui_ImplVulkanH_Window main_window_data_;

    u_ptr<gui::renderer> renderer_up_;

    // for viewport
    VkSampler viewport_sampler_;
    std::vector<VkDescriptorSet> viewport_image_ids_;

    // TODO : make it consistent with hve
    bool swap_chain_rebuild_ = false;
    bool is_gui_engine_running_ = false;

    static ImVec2 viewport_size_;
};
}} // namespace hnll::game