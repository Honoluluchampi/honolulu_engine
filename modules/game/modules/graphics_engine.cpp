// hnll
#include <game/modules/graphics_engine.hpp>
#include <graphics/window.hpp>
#include <graphics/device.hpp>
#include <graphics/renderer.hpp>
#include <graphics/swap_chain.hpp>
#include <utils/rendering_utils.hpp>

namespace hnll::game {

shading_system_map    graphics_engine::shading_systems_;
VkDescriptorSetLayout graphics_engine::global_desc_set_layout_;
VkRenderPass          graphics_engine::default_render_pass_;

u_ptr<graphics_engine> graphics_engine::create(const std::string& window_name, utils::rendering_type rendering_type)
{ return std::make_unique<graphics_engine>(window_name, rendering_type);}

graphics_engine::graphics_engine(const std::string& window_name, utils::rendering_type rendering_type)
{
  window_ = graphics::window::create(WIDTH, HEIGHT, window_name);
  device_ = graphics::device::create(*window_, rendering_type);
  renderer_ = graphics::renderer::create(*window_, *device_);
  init();
}

// delete static vulkan objects explicitly, because static member would be deleted after non-static member(device)
graphics_engine::~graphics_engine()
{

}

// todo : separate into some functions
void graphics_engine::init()
{
  setup_shading_system_config();
  setup_default_shading_systems();
}

void graphics_engine::render()
{
  // returns nullptr if the swap chain is need to be recreated
  if (auto command_buffer = renderer_->begin_frame()) {
    int frame_index = renderer_->get_frame_index();

    renderer_->begin_swap_chain_render_pass(command_buffer, HVE_RENDER_PASS_ID);

    renderer_->end_swap_chain_render_pass(command_buffer);
    renderer_->end_frame();
  }
}

void graphics_engine::wait_idle() { vkDeviceWaitIdle(device_->get_device()); }

void graphics_engine::setup_shading_system_config()
{
  default_render_pass_ = renderer_->get_swap_chain_render_pass(HVE_RENDER_PASS_ID);
  // global_desc_set_layout_ = global_set_layout_->get_descriptor_set_layout();
}

void graphics_engine::setup_default_shading_systems()
{
  auto grid_shader = grid_shading_system::create(*device_);
  add_shading_system(std::move(grid_shader));
}

// getter
bool graphics_engine::should_close_window() const { return window_->should_be_closed(); }

GLFWwindow* graphics_engine::get_glfw_window() const { return window_->get_glfw_window(); }

graphics::window& graphics_engine::get_window_r() { return *window_; }
graphics::device& graphics_engine::get_device_r() { return *device_; }
graphics::renderer& graphics_engine::get_renderer_r() { return *renderer_; }

} // namespace hnll::game