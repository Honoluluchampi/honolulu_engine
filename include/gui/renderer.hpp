#pragma once

// hnll
#include <graphics/renderer.hpp>
#include <graphics/swap_chain.hpp>

namespace hnll {

namespace graphics {
  class image_resource;
}

namespace gui {

#define GUI_RENDER_PASS_ID 1

  class renderer : public hnll::graphics::renderer
    {
      public:
        renderer(graphics::window& window, graphics::device& device, bool recreate_from_scratch);

        renderer(const renderer&) = delete;
        renderer& operator= (const renderer&) = delete;

        static u_ptr<renderer> create(graphics::window& window, graphics::device& device, bool recreate_from_scratch);

        inline VkRenderPass get_render_pass() { return swap_chain_->get_render_pass(GUI_RENDER_PASS_ID); }

        void recreate_swap_chain() override;

        std::vector<VkImageView> get_view_port_image_views() const;

      private:
        // specific for hie
        VkRenderPass create_render_pass();
        std::vector<VkFramebuffer> create_frame_buffers();

        void create_viewport_images();

        // for isolated viewport
        // image, image view and image memory are combined
        std::vector<u_ptr<graphics::image_resource>> vp_images_;

        VkRenderPass vp_render_pass_;
        VkPipeline vp_pipeline_;
        VkCommandPool vp_command_pool_;
        std::vector<VkFramebuffer> vp_frame_buffers_;
        std::vector<VkCommandBuffer> vp_command_buffers_;
    };

}
} // namespace hnll::gui
