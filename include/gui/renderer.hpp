#pragma once

// hnll
#include <graphics/renderer.hpp>
#include <graphics/swap_chain.hpp>

namespace hnll::gui {

#define GUI_RENDER_PASS_ID 1

class renderer : public hnll::graphics::renderer
{
  public:
    renderer(graphics::window& window, graphics::device& device, bool recreate_from_scratch);

    renderer(const renderer&) = delete;
    renderer& operator= (const renderer&) = delete;

    static u_ptr<renderer> create(graphics::window& window, graphics::device& device, bool recreate_from_scratch)
    { return std::make_unique<renderer>(window, device, recreate_from_scratch); }

    inline VkRenderPass get_render_pass() { return swap_chain_->get_render_pass(GUI_RENDER_PASS_ID); }

    void recreate_swap_chain() override;

  private:
    // specific for hie
    VkRenderPass create_render_pass();
    std::vector<VkFramebuffer> create_frame_buffers();
};

} // namespace hnll::gui
