// hnll
#include <graphics/renderer.hpp>
#include <graphics/swap_chain.hpp>
#include <utils/vulkan_config.hpp>

namespace hnll::graphics {

// static members
uint32_t renderer::current_image_index_ = 0;
int renderer::current_frame_index_ = 0;
bool renderer::swap_chain_recreated_ = false;
std::vector<VkCommandBuffer> renderer::submitting_command_buffers_ = {};
u_ptr<swap_chain> renderer::swap_chain_ = nullptr;

renderer::renderer(window& window, device& device, bool recreate_from_scratch)
  : window_{window}, device_{device}
{
  // recreate swap chain dependent objects
  if (recreate_from_scratch) recreate_swap_chain();

  command_buffers_ = device.create_command_buffers(utils::FRAMES_IN_FLIGHT, command_type::GRAPHICS);
  view_port_command_buffers_ = device.create_command_buffers(utils::FRAMES_IN_FLIGHT, command_type::GRAPHICS);
}

renderer::~renderer()
{
  device_.free_command_buffers(std::move(command_buffers_), command_type::GRAPHICS);
  swap_chain_.reset();
}

void renderer::recreate_swap_chain()
{
  // stop calculation until the window is minimized
  auto extent = window_.get_extent();
  while (extent.width == 0 || extent.height == 0) {
    extent = window_.get_extent();
    glfwWaitEvents();
  }
  // wait for finishing the current task
  vkDeviceWaitIdle(device_.get_device());

  // for first creation
  if (swap_chain_ == nullptr)
    swap_chain_ = std::make_unique<swap_chain>(device_, extent);
    // recreate
  else {
    // move the ownership of the current swap chain to old one.
    std::unique_ptr<swap_chain> old_swap_chain = std::move(swap_chain_);
    swap_chain_ = std::make_unique<swap_chain>(device_, extent, std::move(old_swap_chain));
  }
  // if render pass compatible, do nothing else
  // execute this function at the last of derived function's recreate_swap_chain();
  if (next_renderer_) next_renderer_->recreate_swap_chain();

  swap_chain_recreated_ = true;
}

void renderer::reset_frame()
{
  swap_chain_recreated_ = false;
  submitting_command_buffers_.clear();
  // increment current_frame_index_
  if (++current_frame_index_ == utils::FRAMES_IN_FLIGHT)
    current_frame_index_ = 0;
}

void renderer::cleanup_swap_chain()
{
  swap_chain_.reset();
}

bool renderer::begin_frame()
{
  assert(!is_frame_started_ && "Can't call begin_frame() while already in progress");
  // get finished image from swap chain
  // TODO : get isFirstRenderer()
  if (!is_last_renderer()) {
    reset_frame();
    auto result = swap_chain_->acquire_next_image(&current_image_index_);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window_.is_resized()) {
      window_.reset_window_resized_flag();
      recreate_swap_chain();
      // the frame has not successfully started
      return false;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
      throw std::runtime_error("failed to acquire swap chain image!");
  }

  is_frame_started_ = true;
  return true;
}

VkCommandBuffer renderer::begin_command_buffer(int render_id)
{
  VkCommandBuffer command;
  command = get_current_command_buffer();
#ifndef IMGUI_DISABLED
  if (render_id == 1) {
    command = view_port_command_buffers_[current_frame_index_];
  }
#endif
  VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
  if (vkBeginCommandBuffer(command, &begin_info) != VK_SUCCESS)
    throw std::runtime_error("failed to begin recording command buffer.");

  return command;
}

void renderer::record_default_render_command()
{
  auto command = begin_command_buffer(HVE_RENDER_PASS_ID);

  begin_render_pass(command, HVE_RENDER_PASS_ID, swap_chain_->get_swap_chain_extent());

  end_render_pass(command);

  end_frame(command);
  is_frame_started_ = true;
}

void renderer::end_frame(VkCommandBuffer command)
{
  assert(is_frame_started_ && "Can't call end_frame() while the frame is not in progress");
  // end recording command buffer
  if (vkEndCommandBuffer(command) != VK_SUCCESS)
    throw std::runtime_error("failed to record command buffer!");

#ifdef IMGUI_DISABLED
  auto result = swap_chain_->submit_command_buffers(&command_buffer, &current_image_index_);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window_.is_resized()) {
    window_.reset_window_resized_flag();
    recreate_swap_chain();
  }
  else if (result != VK_SUCCESS)
    throw std::runtime_error("failed to present swap chain image!");

  is_frame_started_ = false;
  // increment current_frame_index_
  if (++current_frame_index_ == swap_chain::MAX_FRAMES_IN_FLIGHT)
    current_frame_index_ = 0;
#else
  submitting_command_buffers_.push_back(command);
#endif

  is_frame_started_ = false;

  if (is_last_renderer()) {
    submit_command_buffers();
  }
}

void renderer::begin_render_pass(VkCommandBuffer command_buffer, int renderPassId, VkExtent2D extent)
{
  assert(is_frame_started_ && "Can't call begin_swap_chain_render_pass() while the frame is not in progress.");

  // starting a render pass
  VkRenderPassBeginInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

#ifdef IMGUI_DISABLED
  render_pass_info.renderPass = swap_chain_->get_render_pass();
  render_pass_info.framebuffer = swap_chain_->getFrameBuffer(current_image_index_);
#else
  render_pass_info.renderPass = swap_chain_->get_render_pass(renderPassId);
  render_pass_info.framebuffer = swap_chain_->get_frame_buffer(renderPassId, current_image_index_);
#endif

  // the pixels outside this region will have undefined values
  render_pass_info.renderArea.offset = {0, 0};
  render_pass_info.renderArea.extent = extent;

  // default value for color and depth
  std::array<VkClearValue, 2> clear_values;
#ifndef IMGUI_DISABLED
  if (renderPassId != VIEWPORT_RENDER_PASS_ID)
    clear_values[0].color = {0.007f, 0.007f, 0.01f, 0.5f};
  else
#endif
    clear_values[0].color = {1.f, 1.f, 1.f, 1.f};
  clear_values[1].depthStencil = {1.0f, 0};
  render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
  render_pass_info.pClearValues = clear_values.data();

  // last parameter controls how the drawing commands within the render pass
  // will be provided.
  vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

  // dynamic viewport and scissor
  // transformation of the image
  // draw entire framebuffer
  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(extent.width);
  viewport.height = static_cast<float>(extent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  // cut the region of the framebuffer(swap chain)
  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = extent;
  vkCmdSetViewport(command_buffer, 0, 1, &viewport);
  vkCmdSetScissor(command_buffer, 0, 1, &scissor);
}

void renderer::end_render_pass(VkCommandBuffer command_buffer)
{
  assert(is_frame_started_ && "Can't call end_swap_chain_render_pass() while the frame is not in progress.");

  // finish render pass and recording the command buffer
  vkCmdEndRenderPass(command_buffer);
}

#ifndef IMGUI_DISABLED
void renderer::submit_command_buffers()
{
  auto result = swap_chain_->submit_command_buffers(submitting_command_buffers_.data(), &current_image_index_);
  if (result != VK_ERROR_OUT_OF_DATE_KHR && result != VK_SUBOPTIMAL_KHR && result != VK_SUCCESS)
    throw std::runtime_error("failed to present swap chain image!");
}
#endif

// getter
#ifdef IMGUI_DISABLED
VkRenderPass renderer::get_swap_chain_render_pass() const
{ return swap_chain_->get_render_pass(); }
#else
VkRenderPass renderer::get_swap_chain_render_pass(int render_pass_id) const
{ return swap_chain_->get_render_pass(render_pass_id); }
#endif

float renderer::get_aspect_ratio()         const { return swap_chain_->extent_aspect_ratio(); }
VkImage       renderer::get_image(int index) const { return swap_chain_->get_image(index); }
VkImageView   renderer::get_view(int index)  const { return swap_chain_->get_image_view(index); }

} // namespace hnll::graphics