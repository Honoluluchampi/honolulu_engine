#pragma once

// hnll
#include <graphics/device.hpp>
#include <graphics/window.hpp>

// lib
#include <vulkan/vulkan.h>

namespace hnll::graphics {

class swap_chain;

class renderer
{
  public:

    renderer(window& window, device& device, bool recreate_from_scratch = true);
    virtual ~renderer();

    renderer(const renderer &) = delete;
    renderer &operator= (const renderer &) = delete;

    static u_ptr<renderer> create(window& window, device& device, bool recreate_from_scratch = true)
    { return std::make_unique<renderer>(window, device, recreate_from_scratch); }

    // getter
    VkRenderPass get_swap_chain_render_pass(int render_pass_id) const;
    float        get_aspect_ratio()         const;
    VkImage      get_image(int index) const;
    VkImageView  get_view(int index)  const;
    inline bool is_frame_in_progress()      const { return is_frame_started_; }
    inline swap_chain& get_swap_chain_r()     const { return *swap_chain_; }
    inline VkCommandPool get_command_pool() const { return device_.get_graphics_command_pool(); }

    VkCommandBuffer get_current_command_buffer() const
    {
      // assert(is_frame_started_ && "Cannot get command buffer when frame not in progress");
      return command_buffers_[current_frame_index_];
    }
    int get_frame_index() const
    {
      // assert(is_frame_started_ && "Cannot get frame when frame not in progress");
      return current_frame_index_;
    }

    bool begin_frame();
    void end_frame(VkCommandBuffer command);
    void begin_render_pass(VkCommandBuffer command_buffer, int render_pass_id, VkExtent2D extent);
    void end_render_pass(VkCommandBuffer command_buffer);
    VkCommandBuffer begin_command_buffer(int renderer_id);

    // for default render pass
    void record_default_render_command();

    virtual void recreate_swap_chain();

    inline void set_next_renderer(renderer* renderer)
    { next_renderer_ = renderer; }

    bool is_last_renderer() const
    { return !next_renderer_; }

    static bool swap_chain_recreated_;
    static void cleanup_swap_chain();

  private:

#ifndef IMGUI_DISABLED
    static void submit_command_buffers();
    static void reset_frame();
#endif

  protected:
    window& window_;
    device& device_;
    std::vector<VkCommandBuffer> command_buffers_;

    static uint32_t current_image_index_;
    static int current_frame_index_; // [0, max_frames_in_flight]
    bool is_frame_started_ = false;

#ifndef IMGUI_DISABLED
    std::vector<VkCommandBuffer> view_port_command_buffers_;
    // store multiple renderers' command buffers
    static std::vector<VkCommandBuffer> submitting_command_buffers_;
#endif

    // TODO : use smart pointer
    renderer* next_renderer_ = nullptr;

    static u_ptr<swap_chain> swap_chain_;
};

} // namespace hnll::graphics