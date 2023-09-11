// hnll
#include <game/modules/graphics_engine.hpp>
#include <graphics/window.hpp>
#include <graphics/device.hpp>
#include <graphics/buffer.hpp>
#include <graphics/renderer.hpp>
#include <graphics/swap_chain.hpp>
#include <graphics/desc_set.hpp>
#include <utils/vulkan_config.hpp>
#include <utils/singleton.hpp>

namespace hnll::game {

// ************************ graphics engine core ************************************
// static members

u_ptr<graphics::renderer> graphics_engine_core::renderer_;

s_ptr<graphics::desc_pool>           graphics_engine_core::global_pool_;
u_ptr<graphics::desc_sets>           graphics_engine_core::global_desc_sets_;

VkRenderPass          graphics_engine_core::default_render_pass_;

u_ptr<graphics::graphics_model_pool> graphics_engine_core::model_pool_;

graphics_engine_core::graphics_engine_core(const std::string& window_name, utils::rendering_type rendering_type)
 : window_(utils::singleton<graphics::window>::get_instance(WIDTH, HEIGHT, window_name)),
   device_(utils::singleton<graphics::device>::get_instance(rendering_type))
{
  renderer_ = graphics::renderer::create(window_, device_);
  model_pool_ = graphics::graphics_model_pool::create(device_);
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
  global_pool_ = graphics::desc_pool::builder(device_)
    .set_max_sets(utils::FRAMES_IN_FLIGHT)
    .set_pool_flags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
    .add_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, utils::FRAMES_IN_FLIGHT)
    .build();

  // build desc sets
  graphics::desc_set_info global_set_info;
  // enable ubo to be referenced by all stages of a graphics pipeline
  global_set_info.add_binding(
    VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_MESH_BIT_NV | VK_SHADER_STAGE_RAYGEN_BIT_KHR,
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

  global_desc_sets_ = graphics::desc_sets::create(
    device_,
    global_pool_,
    {global_set_info},
    utils::FRAMES_IN_FLIGHT);

  // creating ubo for each frame version
  for (int i = 0; i < utils::FRAMES_IN_FLIGHT; i++) {
    auto buf = graphics::buffer::create(
      device_,
      sizeof(utils::global_ubo),
      1,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      nullptr
    );
    global_desc_sets_->set_buffer(0, 0, i, std::move(buf));
  }

  global_desc_sets_->build();
}

void graphics_engine_core::cleanup()
{
  graphics::texture_image::reset_desc_layout();
  model_pool_.reset();
  global_desc_sets_.reset();
  global_pool_.reset();
  renderer_.reset();
}

void graphics_engine_core::wait_idle() { vkDeviceWaitIdle(device_.get_device()); }

// for graphics_engine::render()
bool graphics_engine_core::begin_frame() { return renderer_->begin_frame(); }
void graphics_engine_core::record_default_render_command()
{ renderer_->record_default_render_command(); }

VkCommandBuffer graphics_engine_core::begin_command_buffer(int renderer_id)
{ return renderer_->begin_command_buffer(renderer_id); }

int graphics_engine_core::get_frame_index() { return renderer_->get_frame_index(); }

void graphics_engine_core::begin_render_pass(VkCommandBuffer command_buffer, int renderer_id, VkExtent2D extent)
{ renderer_->begin_render_pass(command_buffer, renderer_id, extent); }

void graphics_engine_core::end_render_pass_and_frame(VkCommandBuffer command_buffer)
{
  renderer_->end_render_pass(command_buffer);
  renderer_->end_frame(command_buffer);
}

void graphics_engine_core::end_frame(VkCommandBuffer command_buffer)
{ renderer_->end_frame(command_buffer); }

void graphics_engine_core::setup_global_shading_system_config()
{
  default_render_pass_ = renderer_->get_swap_chain_render_pass(HVE_RENDER_PASS_ID);

  // texture desc layout should be defined before shading system ctor
  graphics::texture_image::setup_desc_layout(device_);
}

// getter
bool graphics_engine_core::should_close_window() const { return window_.should_be_closed(); }

VkDescriptorSet graphics_engine_core::update_ubo(const utils::global_ubo& ubo, int frame_index)
{
  global_desc_sets_->write_to_buffer(0, 0, frame_index, (void *)&ubo);
  global_desc_sets_->flush_buffer(0, 0, frame_index);
  return global_desc_sets_->get_vk_desc_sets(frame_index)[0];
}

GLFWwindow* graphics_engine_core::get_glfw_window() const { return window_.get_glfw_window(); }

graphics::renderer& graphics_engine_core::get_renderer_r() { return *renderer_; }
graphics::timeline_semaphore& graphics_engine_core::get_compute_semaphore_r()
{ return renderer_->get_swap_chain_r().get_compute_semaphore_r(); }

VkDescriptorSetLayout graphics_engine_core::get_global_desc_layout()
{ return global_desc_sets_->get_vk_layouts()[0]; }

std::tuple<int, int> graphics_engine_core::get_window_size()
{
  auto extent = utils::singleton<graphics::window>::get_instance().get_extent();
  std::tuple<int, int> ret { extent.width, extent.height };
  return ret;
}

} // namespace hnll::game