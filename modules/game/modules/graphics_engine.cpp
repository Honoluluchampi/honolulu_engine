// hnll
#include <game/modules/graphics_engine.hpp>
#include <graphics/window.hpp>
#include <graphics/device.hpp>
#include <graphics/renderer.hpp>
#include <graphics/swap_chain.hpp>
#include <graphics/desc_set.hpp>
#include <utils/rendering_utils.hpp>

namespace hnll::game {

u_ptr<graphics::window>   graphics_engine_core::window_;
u_ptr<graphics::device>   graphics_engine_core::device_;
u_ptr<graphics::renderer> graphics_engine_core::renderer_;
u_ptr<graphics::desc_layout> graphics_engine_core::global_set_layout_;

// ************************ graphics engine core ************************************
// static members
VkDescriptorSetLayout graphics_engine_core::vk_global_desc_layout_;
VkRenderPass          graphics_engine_core::default_render_pass_;

graphics_engine_core::graphics_engine_core(const std::string& window_name, utils::rendering_type rendering_type)
{
  window_ = graphics::window::create(WIDTH, HEIGHT, window_name);
  device_ = graphics::device::create(*window_, rendering_type);
  renderer_ = graphics::renderer::create(*window_, *device_);
  init();
}

// delete static vulkan objects explicitly, because static member would be deleted after non-static member(device)
graphics_engine_core::~graphics_engine_core()
{

}

// todo : separate into some functions
void graphics_engine_core::init()
{
  setup_ubo();
  setup_global_shading_system_config();
}

void graphics_engine_core::setup_ubo()
{
  // this is set layout of master system
  // enable ubo to be referenced by oall stages of a graphics pipeline
  global_set_layout_ = graphics::desc_layout::builder(*device_)
    .add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_MESH_BIT_NV)
    .build();
  // may add additional layout of child system
}

void graphics_engine_core::wait_idle() { vkDeviceWaitIdle(device_->get_device()); }

// for graphics_engine::render()
VkCommandBuffer graphics_engine_core::begin_frame() { return renderer_->begin_frame(); }

int graphics_engine_core::get_frame_index() { return renderer_->get_frame_index(); }

void graphics_engine_core::begin_swap_chain_render_pass(VkCommandBuffer command_buffer)
{ renderer_->begin_swap_chain_render_pass(command_buffer, HVE_RENDER_PASS_ID); }

void graphics_engine_core::end_swap_chain_and_frame(VkCommandBuffer command_buffer)
{ renderer_->end_swap_chain_render_pass(command_buffer); renderer_->end_frame(); }

void graphics_engine_core::setup_global_shading_system_config()
{
  default_render_pass_ = renderer_->get_swap_chain_render_pass(HVE_RENDER_PASS_ID);
  vk_global_desc_layout_ = global_set_layout_->get_descriptor_set_layout();
}

// getter
bool graphics_engine_core::should_close_window() const { return window_->should_be_closed(); }

GLFWwindow* graphics_engine_core::get_glfw_window() const { return window_->get_glfw_window(); }

graphics::window& graphics_engine_core::get_window_r() { return *window_; }
graphics::device& graphics_engine_core::get_device_r() { return *device_; }
graphics::renderer& graphics_engine_core::get_renderer_r() { return *renderer_; }

} // namespace hnll::game