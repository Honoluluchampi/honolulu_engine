#pragma once

// hnll
#include <graphics/renderer.hpp>
#include <graphics/swap_chain.hpp>

namespace hnll {

namespace graphics {
  class image_resource;
  class desc_pool;
  class desc_layout;
}

namespace gui {

#define GUI_RENDER_PASS_ID 2

  class renderer : public hnll::graphics::renderer
    {
      public:
        renderer(
          graphics::window& window,
          graphics::device& device,
          bool recreate_from_scratch);

        renderer(const renderer&) = delete;

        renderer& operator= (const renderer&) = delete;

        static u_ptr<renderer> create(
          graphics::window& window,
          graphics::device& device,
          bool recreate_from_scratch);

        void recreate_swap_chain() override;

        inline VkRenderPass get_render_pass() { return swap_chain_->get_render_pass(GUI_RENDER_PASS_ID); }
        std::vector<VkImageView> get_view_port_image_views() const;

        // for gui engine
        inline static float get_left_window_ratio() { return left_window_ratio_; }
        inline static float get_bottom_window_ratio() { return bottom_window_ratio_; }
        inline static float* get_left_window_ratio_p() { return &left_window_ratio_; }
        inline static float* get_bottom_window_ratio_p() { return &bottom_window_ratio_; }

        // for ray tracing
        inline VkDescriptorSetLayout get_vp_image_desc_layout() { return vk_desc_layout_; }
        inline std::vector<VkDescriptorSet> get_vp_image_desc_sets() { return vp_image_descs_; }

        void transition_vp_image_layout(int frame_index, VkImageLayout new_layout, VkCommandBuffer command);
    private:
        // specific for hie
        VkRenderPass create_viewport_render_pass();
        VkRenderPass create_imgui_render_pass();
        std::vector<VkFramebuffer> create_viewport_frame_buffers();
        std::vector<VkFramebuffer> create_imgui_frame_buffers();

        void create_viewport_images();

        // for isolated viewport
        // image, image view and image memory are combined
        std::vector<u_ptr<graphics::image_resource>> vp_images_;
        std::vector<VkCommandBuffer> vp_command_buffers_;

        // for ray tracing desc image
        u_ptr<graphics::desc_layout> desc_layout_;
        VkDescriptorSetLayout vk_desc_layout_;
        s_ptr<graphics::desc_pool> desc_pool_;
        std::vector<VkDescriptorSet> vp_image_descs_;

        utils::rendering_type rendering_type_;

        static float left_window_ratio_;
        static float bottom_window_ratio_;
    };
}
} // namespace hnll::gui
