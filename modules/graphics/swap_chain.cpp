//hnll
#include <graphics/swap_chain.hpp>
#include <graphics/timeline_semaphore.hpp>
#include <utils/rendering_utils.hpp>

// std
#include <array>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace hnll::graphics {

swap_chain::swap_chain(device &device, VkExtent2D extent, u_ptr<swap_chain>&& previous)
  : device_{device}, window_extent_{extent}, old_swap_chain_(std::move(previous))
{
  init();

  // derive compute semaphore
  if (old_swap_chain_ != nullptr) {
    move_timeline_semaphores();
  }
  else {
    compute_semaphore_ = timeline_semaphore::create(device_);
  }

  // clean up old swap chain since it's no longer needed
  old_swap_chain_.reset();
}

void swap_chain::init()
{
  create_swap_chain();
  create_image_views();

#ifdef IMGUI_DISABLED
  render_pass_ = create_render_pass();
  create_depth_resources();
  swapChain_framebuffers_ = create_frame_buffers(renderPass_);
#else
  multiple_frame_buffers_.resize(RENDERER_COUNT);
  multiple_render_pass_.resize(RENDERER_COUNT);
  create_multiple_render_pass();
  create_depth_resources();
  create_multiple_frame_buffers();
#endif

  create_sync_objects();
}

swap_chain::~swap_chain()
{
  for (auto image_view : swap_chain_image_views_) {
    vkDestroyImageView(device_.get_device(), image_view, nullptr);
  }
  swap_chain_image_views_.clear();

  if (swap_chain_ != nullptr) {
    vkDestroySwapchainKHR(device_.get_device(), swap_chain_, nullptr);
    swap_chain_ = nullptr;
  }

  for (int i = 0; i < depth_images_.size(); i++) {
    vkDestroyImageView(device_.get_device(), depth_image_views_[i], nullptr);
    vkDestroyImage(device_.get_device(), depth_images_[i], nullptr);
    vkFreeMemory(device_.get_device(), depth_image_memories_[i], nullptr);
  }

#ifdef IMGUI_DISABLED
  for (auto framebuffer : swap_chain_framebuffers_) {
    vkDestroyFramebuffer(device_.get_device(), framebuffer, nullptr);
  }
  vkDestroyRenderPass(device_.get_device(), render_pass_, nullptr);
#else
  for (auto framebuffers : multiple_frame_buffers_) {
    for (auto framebuffer : framebuffers) {
      vkDestroyFramebuffer(device_.get_device(), framebuffer, nullptr);
    }
  }
  for (auto render_pass : multiple_render_pass_)
    vkDestroyRenderPass(device_.get_device(), render_pass, nullptr);
#endif

  // cleanup synchronization objects
  for (size_t i = 0; i < utils::FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(device_.get_device(), render_finished_semaphores_[i], nullptr);
    vkDestroySemaphore(device_.get_device(), image_available_semaphores_[i], nullptr);
    vkDestroyFence(device_.get_device(), in_flight_fences_[i], nullptr);
  }
}

VkResult swap_chain::acquire_next_image(uint32_t *image_index)
{
  vkWaitForFences(device_.get_device(), 1, &in_flight_fences_[current_frame_],
                  VK_TRUE, std::numeric_limits<uint64_t>::max());

  VkResult result = vkAcquireNextImageKHR(
    device_.get_device(),
    swap_chain_,
    std::numeric_limits<uint64_t>::max(),
    image_available_semaphores_[current_frame_],  // must be a not signaled semaphore
    VK_NULL_HANDLE,
    image_index);

  return result;
}

// buffers : default command buffer, viewport command buffer, imgui command buffer
VkResult swap_chain::submit_command_buffers(const VkCommandBuffer *buffers, uint32_t *image_index)
{
  // specify a timeout in nanoseconds for an image
  auto timeout = UINT64_MAX;
  // check if a previous frame is using this image
  if(images_in_flight_[*image_index] != VK_NULL_HANDLE)
    vkWaitForFences(device_.get_device(), 1, &images_in_flight_[*image_index], VK_TRUE, timeout);
  // mark the image as now being in use by this frame
  images_in_flight_[*image_index] = in_flight_fences_[current_frame_];

  // describe wait semaphores
  VkSemaphoreSubmitInfoKHR wait_semaphores[] {
    { // image available semaphore
      VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR,
      nullptr,
      image_available_semaphores_[current_frame_],
      0,
      VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
      0
    },
    { // compute semaphore
      VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR,
      nullptr,
      compute_semaphore_->get_vk_semaphore(),
      compute_semaphore_->get_last_semaphore_value(),
      VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT_KHR,
      0
    }
  };

  //submitting the command buffer
  VkSubmitInfo2KHR submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR;
  submit_info.waitSemaphoreInfoCount = 2;
  submit_info.pWaitSemaphoreInfos = wait_semaphores;
  // which command buffers to actually submit for execution
  // should submit the command buffer that binds the swap chain image
  // we just acquired as color attachment.

  // TODO : configure renderer count in a systematic way
#ifdef IMGUI_DISABLED
  VkCommandBufferSubmitInfoKHR command_buffers[] {
    {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR,
      nullptr,
      buffers[0],
      0
    },
  };

  submit_info.commandBufferInfoCount = 1;
  submit_info.pCommandBufferInfos = command_buffers;
#else
  VkCommandBufferSubmitInfoKHR command_buffers[] {
    {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR,
      nullptr,
      buffers[0],
      0
    },
    {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR,
      nullptr,
      buffers[1],
      0
    }
  };

  submit_info.commandBufferInfoCount = 2;
  submit_info.pCommandBufferInfos = command_buffers;
#endif
  // specify which semaphores to signal once the command buffer have finished execution
  VkSemaphoreSubmitInfoKHR signal_semaphores[] {
    {
      VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR,
      nullptr,
      render_finished_semaphores_[current_frame_],
      0,
      VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT_KHR,
      0
    }
  };

  submit_info.signalSemaphoreInfoCount = 1;
  submit_info.pSignalSemaphoreInfos = signal_semaphores;

  // need to be manually restore the fence to the unsignaled state
  vkResetFences(device_.get_device(), 1, &in_flight_fences_[current_frame_]);

  // submit the command buffer to the graphics queue with fence
  if (vkQueueSubmit2(device_.get_graphics_queue(), 1, &submit_info, in_flight_fences_[current_frame_]) != VK_SUCCESS)
    throw std::runtime_error("failed to submit draw command buffer!");

  // configure sub-pass dependencies in VkRenderPassFactory::create_render_pass

  // presentation
  // submit the result back to the swap chain to have it eventually show up on the screen
  VkPresentInfoKHR present_info = {};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &render_finished_semaphores_[current_frame_];

  VkSwapchainKHR swapChains[] = {swap_chain_};
  present_info.swapchainCount = 1;
  present_info.pSwapchains = swapChains;
  present_info.pImageIndices = image_index;
  // necessary for multi swap chain
  // present_info.pResults = nullptr;

  auto result = vkQueuePresentKHR(device_.get_present_queue(), &present_info);

  current_frame_ = (current_frame_ + 1) % utils::FRAMES_IN_FLIGHT;

  return result;
}

void swap_chain::create_swap_chain()
{
  swap_chain_support_details swap_chain_support = device_.get_swap_chain_support();

  surface_format_ = choose_swap_surface_format(swap_chain_support.formats_);
  auto present_mode = choose_swap_present_mode(swap_chain_support.present_modes_);
  auto extent = choose_swap_extent(swap_chain_support.capabilities_);
  // how many images id like to have in the swap chain
  uint32_t get_image_count = swap_chain_support.capabilities_.minImageCount + 1;
  // make sure to not exceed the maximum number of images
  if (swap_chain_support.capabilities_.maxImageCount > 0 &&
      get_image_count > swap_chain_support.capabilities_.maxImageCount) {
    get_image_count = swap_chain_support.capabilities_.maxImageCount;
  }
  // fill in a structure with required information
  VkSwapchainCreateInfoKHR create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.surface = device_.get_surface();

  create_info.minImageCount = get_image_count;
  create_info.imageFormat = surface_format_.format;
  create_info.imageColorSpace = surface_format_.colorSpace;
  create_info.imageExtent = extent;
  // the amount of layers each image consists of
  create_info.imageArrayLayers = 1;
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  // specify how to handle swap chain images by multiple queue families
  queue_family_indices indices = device_.find_physical_queue_families();
  uint32_t get_queue_family_indices[] = {indices.graphics_family_.value(), indices.present_family_.value()};

  if (indices.graphics_family_.value() != indices.present_family_.value()) {
    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = 2;
    create_info.pQueueFamilyIndices = get_queue_family_indices;
  } else {
    // this option offers best performance
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0;      // Optional
    create_info.pQueueFamilyIndices = nullptr;  // Optional
  }

  // like a 90 degree clockwise rotation or horizontal flip
  create_info.preTransform = swap_chain_support.capabilities_.currentTransform;
  // ignore alpha channel
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_info.presentMode = present_mode;
  // ignore the color of obscured pixels
  create_info.clipped = VK_TRUE;
  // invalid or unoptimized swap chain should be reacreated from scratch
  create_info.oldSwapchain = old_swap_chain_ == nullptr ? VK_NULL_HANDLE : old_swap_chain_->swap_chain_;

  // create swap chain
  if (vkCreateSwapchainKHR(device_.get_device(), &create_info, nullptr, &swap_chain_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create swap chain!");
  }

  // we only specified a minimum number of images in the swap chain, so the implementation is
  // allowed to create a swap chain with more. That's why we'll first query the final number of
  // images with vkGetSwapchainImagesKHR, then resize the container and finally call it again to
  // retrieve the handles.
  vkGetSwapchainImagesKHR(device_.get_device(), swap_chain_, &get_image_count, nullptr);
  swap_chain_images_.resize(get_image_count);
  vkGetSwapchainImagesKHR(device_.get_device(), swap_chain_, &get_image_count, swap_chain_images_.data());

  swap_chain_image_format_ = surface_format_.format;
  swap_chain_extent_ = extent;
}

void swap_chain::create_image_views()
{
  // create image view for all VkImage in the swap chain
  swap_chain_image_views_.resize(swap_chain_images_.size());
  for (size_t i = 0; i < swap_chain_images_.size(); i++) {
    VkImageViewCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = swap_chain_images_[i];
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = swap_chain_image_format_;
    // swizzle the color channeld around
    create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    // subresourceRange describes what images purpose is
    // and which part of the image should be accessed
    create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device_.get_device(), &create_info, nullptr,
                          &swap_chain_image_views_[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create image views!");
    }
  }
}

VkRenderPass swap_chain::create_render_pass()
{
  VkAttachmentDescription color_attachment = {};
  color_attachment.format = get_swap_chain_images_format();
  color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  // what to do with the data in the attachment before and after rendering
  // clear the framebuffer to black before drawing a new frame
  color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

#ifdef IMGUI_DISABLED
  color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
#else
  color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
#endif

  VkAttachmentDescription depth_attachment{};
  depth_attachment.format = find_depth_format();
  depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
#ifndef IMGUI_DISABLED
  depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
#else
  depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
#endif
  depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference color_attachment_ref = {};
  // which attachment to reference by its index
  color_attachment_ref.attachment = 0;
  // which layout we would like the attachment to have during a subpass
  color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depth_attachmet_ref{};
  depth_attachmet_ref.attachment = 1;
  depth_attachmet_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  // subpass creation
  // graphics may support compute subpasses in the future
  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_attachment_ref;
  subpass.pDepthStencilAttachment = &depth_attachmet_ref;

  std::vector<VkSubpassDependency> dependencies = {};
#ifndef IMGUI_DISABLED
  dependencies.resize(2);
  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[1].srcSubpass = 0;
  dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
#elif
  dependencies.resize(1);
  // the implicit subpass befor or after the render pass
  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].srcAccessMask = 0;
  // the operations to wait on and the stages in which these operations occur
  dependencies[0].srcStageMask =
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  // dstSubpass souhld be higher than srcSubpass to prevent cycles
  dependencies[0].dstSubpass = 0;
  dependencies[0].dstStageMask =
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependencies[0].dstAccessMask =
    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
#endif

  std::array<VkAttachmentDescription, 2> attachments = {color_attachment, depth_attachment};
  VkRenderPassCreateInfo render_pass_info = {};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
  render_pass_info.pAttachments = attachments.data();
  // attachment and subpass can be array of those;
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &subpass;
  render_pass_info.dependencyCount = static_cast<uint32_t>(dependencies.size());
  render_pass_info.pDependencies = dependencies.data();

  VkRenderPass render_pass;

  if (vkCreateRenderPass(device_.get_device(), &render_pass_info, nullptr, &render_pass) != VK_SUCCESS) {
    throw std::runtime_error("failed to create render pass!");
  }

  return render_pass;
}

std::vector<VkFramebuffer> swap_chain::create_frame_buffers(VkRenderPass render_pass)
{
  std::vector<VkFramebuffer> swap_chain_frame_buffers(get_image_count());
  for (size_t i = 0; i < get_image_count(); i++) {
    std::array<VkImageView, 2> attachments = {swap_chain_image_views_[i], depth_image_views_[i]};

    VkExtent2D swap_chain_extent = get_swap_chain_extent();
    VkFramebufferCreateInfo frame_buffer_info = {};
    frame_buffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    // render_pass the framebuffer needs to be compatible
    frame_buffer_info.renderPass = render_pass;
    frame_buffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    frame_buffer_info.pAttachments = attachments.data();
    frame_buffer_info.width = swap_chain_extent.width;
    frame_buffer_info.height = swap_chain_extent.height;
    frame_buffer_info.layers = 1;

    if (vkCreateFramebuffer(device_.get_device(), &frame_buffer_info, nullptr,
                            &swap_chain_frame_buffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }

  return swap_chain_frame_buffers;
}

void swap_chain::create_multiple_render_pass()
{ multiple_render_pass_[HVE_RENDER_PASS_ID] = create_render_pass(); }

void swap_chain::create_multiple_frame_buffers()
{ multiple_frame_buffers_[HVE_RENDER_PASS_ID] = create_frame_buffers(multiple_render_pass_[HVE_RENDER_PASS_ID]); }

void swap_chain::reset_frame_buffers(int render_pass_id)
{
  if (multiple_frame_buffers_[render_pass_id].size() > 0) {
    vkDeviceWaitIdle(device_.get_device());
    for (auto& framebuffer : multiple_frame_buffers_[render_pass_id])
      vkDestroyFramebuffer(device_.get_device(), framebuffer, nullptr);
  }
}

void swap_chain::reset_render_pass(int render_pass_id)
{
  if (multiple_render_pass_[render_pass_id] != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(device_.get_device());
    vkDestroyRenderPass(device_.get_device(), multiple_render_pass_[render_pass_id], nullptr);
  }
}

void swap_chain::move_timeline_semaphores()
{
  compute_semaphore_ = std::move(old_swap_chain_->compute_semaphore_);
}

void swap_chain::create_depth_resources()
{
  swap_chain_depth_format_ = find_depth_format();
  VkExtent2D swap_chain_extent = get_swap_chain_extent();

  depth_images_.resize(get_image_count());
  depth_image_memories_.resize(get_image_count());
  depth_image_views_.resize(get_image_count());

  for (int i = 0; i < depth_images_.size(); i++) {
    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = swap_chain_extent.width;
    image_info.extent.height = swap_chain_extent.height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = swap_chain_depth_format_;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.flags = 0;

    device_.create_image_with_info(
      image_info,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      depth_images_[i],
      depth_image_memories_[i]);

    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = depth_images_[i];
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = swap_chain_depth_format_;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device_.get_device(), &view_info, nullptr, &depth_image_views_[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create texture image view!");
    }
  }
}

void swap_chain::create_sync_objects()
{
  image_available_semaphores_.resize(utils::FRAMES_IN_FLIGHT);
  render_finished_semaphores_.resize(utils::FRAMES_IN_FLIGHT);
  in_flight_fences_.resize(utils::FRAMES_IN_FLIGHT);
  // initially not a single framce is using an image, so initialize it to no fence
  images_in_flight_.resize(get_image_count(), VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphore_info = {};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fence_info = {};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  // initialize fences in the signaled state as if they had been rendered an initial frame
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < utils::FRAMES_IN_FLIGHT; i++) {
    // future version of the graphics api may add functionality for other parameters
    if (vkCreateSemaphore(device_.get_device(), &semaphore_info, nullptr, &image_available_semaphores_[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device_.get_device(), &semaphore_info, nullptr, &render_finished_semaphores_[i]) != VK_SUCCESS ||
        vkCreateFence(device_.get_device(), &fence_info, nullptr, &in_flight_fences_[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create synchronization objects for a frame!");
    }
  }
}

VkSurfaceFormatKHR swap_chain::choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR> &available_formats)
{
  for (const auto &available_format : available_formats) {
    if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
        available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return available_format;
    }
  }

  return available_formats[0];
}

VkPresentModeKHR swap_chain::choose_swap_present_mode(const std::vector<VkPresentModeKHR> &available_present_modes)
{
  // animation becomes storange if use present mode : mailbox

  for (const auto &available_present_mode : available_present_modes) {
    // prefer triple buffering
    // high cost
    if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      // std::cout << "Present mode: Mailbox" << std::endl;
      return available_present_mode;
    }
  }

  for (const auto &available_present_mode : available_present_modes) {
    if (available_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
      std::cout << "Present mode: Immediate" << std::endl;
      return available_present_mode;
    }
  }

  std::cout << "Present mode: V-Sync" << std::endl;
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D swap_chain::choose_swap_extent(const VkSurfaceCapabilitiesKHR &capabilities)
{
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    VkExtent2D actual_extent = window_extent_;
    actual_extent.width = std::max(capabilities.minImageExtent.width,
                                   std::min(capabilities.maxImageExtent.width, actual_extent.width));
    actual_extent.height = std::max(capabilities.minImageExtent.height,
                                    std::min(capabilities.maxImageExtent.height, actual_extent.height));
    return actual_extent;
  }
}

VkFormat swap_chain::find_depth_format()
{
  return device_.find_supported_format(
    {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
    VK_IMAGE_TILING_OPTIMAL,
    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

} // namespace hnll::graphics