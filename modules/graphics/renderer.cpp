// hnll
#include <graphics/renderer.hpp>
#include <graphics/swap_chain.hpp>

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
  create_command_buffers();
}

renderer::~renderer()
{
  free_command_buffers();
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
  if (++current_frame_index_ == swap_chain::MAX_FRAMES_IN_FLIGHT)
    current_frame_index_ = 0;
}

void renderer::create_command_buffers()
{
  // 2 or 3
  command_buffers_.resize(swap_chain::MAX_FRAMES_IN_FLIGHT);

  // specify command pool and number of buffers to allocate
  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  // if the allocated command buffers are primary or secondary command buffers
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandPool = device_.get_command_pool();
  alloc_info.commandBufferCount = static_cast<uint32_t>(command_buffers_.size());

  if (vkAllocateCommandBuffers(device_.get_device(), &alloc_info, command_buffers_.data()) != VK_SUCCESS)
    throw std::runtime_error("failed to allocate command buffers!");
}

void renderer::free_command_buffers()
{
  vkFreeCommandBuffers(
    device_.get_device(),
    device_.get_command_pool(),
    static_cast<float>(command_buffers_.size()),
    command_buffers_.data());
  command_buffers_.clear();
}

void renderer::cleanup_swap_chain()
{
  swap_chain_.reset();
}

VkCommandBuffer renderer::begin_frame()
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
      return nullptr;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
      throw std::runtime_error("failed to acquire swap chain image!");
  }

  is_frame_started_ = true;

  auto command_buffer = get_current_command_buffer();
  // assert(command_buffer != VK_NULL_HANDLE && "");
  // start recording command buffers
  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  // // how to use the command buffer
  // begin_info.flags = 0;
  // // state to inherit from the calling primary command buffers
  // // (only relevant for secondary command buffers)
  // begin_info.pInheritanceInfo = nullptr;

  if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
    throw std::runtime_error("failed to begin recording command buffer!");
  return command_buffer;
}

void renderer::end_frame()
{
  assert(is_frame_started_ && "Can't call end_frame() while the frame is not in progress");
  auto command_buffer = get_current_command_buffer();
  // end recording command buffer
  if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
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
  submitting_command_buffers_.push_back(command_buffer);
#endif

  is_frame_started_ = false;

  if (is_last_renderer()) {
    submit_command_buffers();
  }
}

void renderer::begin_swap_chain_render_pass(VkCommandBuffer command_buffer, int renderPassId)
{
  assert(is_frame_started_ && "Can't call begin_swap_chain_render_pass() while the frame is not in progress.");
  assert(command_buffer == get_current_command_buffer() && "Can't beginig render pass on command buffer from a different frame.");

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
  render_pass_info.renderArea.extent = swap_chain_->get_swap_chain_extent();

  // default value for color and depth
  std::array<VkClearValue, 2> clear_values;
  clear_values[0].color = {1.f, 1.f, 1.f, 0.5f};
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
  viewport.width = static_cast<float>(swap_chain_->get_swap_chain_extent().width);
  viewport.height = static_cast<float>(swap_chain_->get_swap_chain_extent().height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  // cut the region of the framebuffer(swap chain)
  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swap_chain_->get_swap_chain_extent();
  vkCmdSetViewport(command_buffer, 0, 1, &viewport);
  vkCmdSetScissor(command_buffer, 0, 1, &scissor);
}

void renderer::end_swap_chain_render_pass(VkCommandBuffer command_buffer)
{
  assert(is_frame_started_ && "Can't call end_swap_chain_render_pass() while the frame is not in progress.");
  assert(command_buffer == get_current_command_buffer() && "Can't ending render pass on command buffer from a different frame.");

  // finish render pass and recording the comand buffer
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