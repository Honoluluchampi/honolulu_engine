// hnll
#include <game/modules/graphics_engine.hpp>
#include <graphics/window.hpp>
#include <graphics/device.hpp>
#include <graphics/buffer.hpp>
#include <graphics/renderer.hpp>
#include <graphics/swap_chain.hpp>
#include <graphics/desc_set.hpp>
#include <utils/rendering_utils.hpp>

namespace hnll::game {

// ************************ graphics engine core ************************************
// static members

u_ptr<graphics::window>   graphics_engine_core::window_;
u_ptr<graphics::device>   graphics_engine_core::device_;
u_ptr<graphics::renderer> graphics_engine_core::renderer_;

u_ptr<graphics::desc_layout>         graphics_engine_core::global_set_layout_;
u_ptr<graphics::desc_pool>           graphics_engine_core::global_pool_;
std::vector<u_ptr<graphics::buffer>> graphics_engine_core::ubo_buffers_;
std::vector<VkDescriptorSet>         graphics_engine_core::global_desc_sets_;

VkDescriptorSetLayout graphics_engine_core::vk_global_desc_layout_;
VkRenderPass          graphics_engine_core::default_render_pass_;

u_ptr<graphics::graphics_model_pool> graphics_engine_core::model_pool_;

graphics_engine_core::graphics_engine_core(const std::string& window_name, utils::rendering_type rendering_type)
{
  window_ = graphics::window::create(WIDTH, HEIGHT, window_name);
  device_ = graphics::device::create(*window_, rendering_type);
  renderer_ = graphics::renderer::create(*window_, *device_);
  model_pool_ = graphics::graphics_model_pool::create(*device_);
  init();
}

// delete static vulkan objects explicitly, because static member would be deleted after non-static member(device)
graphics_engine_core::~graphics_engine_core()
{
  cleanup();
}

// todo : separate into some functions
void graphics_engine_core::init()
{
  setup_ubo();
  setup_global_shading_system_config();
}

void graphics_engine_core::setup_ubo()
{
  // // 2 uniform buffer descriptor
  global_pool_ = graphics::desc_pool::builder(*device_)
    .set_max_sets(graphics::swap_chain::MAX_FRAMES_IN_FLIGHT)
    .set_pool_flags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
    .add_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, graphics::swap_chain::MAX_FRAMES_IN_FLIGHT)
    .build();

  ubo_buffers_.resize(graphics::swap_chain::MAX_FRAMES_IN_FLIGHT);
  // creating ubo for each frame version
  for (int i = 0; i < ubo_buffers_.size(); i++) {
    ubo_buffers_[i] = graphics::buffer::create(
      *device_,
      sizeof(utils::global_ubo),
      1,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
      nullptr
    );
  }

  // this is set layout of master system
  // enable ubo to be referenced by oall stages of a graphics pipeline
  global_set_layout_ = graphics::desc_layout::builder(*device_)
    .add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_MESH_BIT_NV)
    .build();
  // may add additional layout of child system

  global_desc_sets_.resize(graphics::swap_chain::MAX_FRAMES_IN_FLIGHT);
  for (int i = 0; i < global_desc_sets_.size(); i++) {
    auto bufferInfo = ubo_buffers_[i]->desc_info();
    graphics::desc_writer(*global_set_layout_, *global_pool_)
      .write_buffer(0, &bufferInfo)
      .build(global_desc_sets_[i]);
  }
}

void graphics_engine_core::cleanup()
{
  global_pool_->free_descriptors(global_desc_sets_);
  for(auto& buffer : ubo_buffers_) buffer.reset();
  global_pool_.reset();
  global_set_layout_.reset();
  renderer_.reset();
  device_.reset();
  window_.reset();
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

VkDescriptorSet graphics_engine_core::update_ubo(const utils::global_ubo& ubo, int frame_index)
{
  ubo_buffers_[frame_index]->write_to_buffer((void *)&ubo);
  ubo_buffers_[frame_index]->flush();
  return global_desc_sets_[frame_index];
}

GLFWwindow* graphics_engine_core::get_glfw_window() const { return window_->get_glfw_window(); }

graphics::window& graphics_engine_core::get_window_r() { return *window_; }
graphics::device& graphics_engine_core::get_device_r() { return *device_; }
graphics::renderer& graphics_engine_core::get_renderer_r() { return *renderer_; }

} // namespace hnll::game