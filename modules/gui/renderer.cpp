// hnll
#include <gui/renderer.hpp>
#include <graphics/image_resource.hpp>

namespace hnll::gui {

u_ptr<renderer> renderer::create(graphics::window& window, graphics::device& device, bool recreate_from_scratch)
{ return std::make_unique<renderer>(window, device, recreate_from_scratch); }

renderer::renderer(graphics::window& window, graphics::device& device, bool recreate_from_scratch) :
  hnll::graphics::renderer(window, device, recreate_from_scratch)
{ recreate_swap_chain(); }

void renderer::recreate_swap_chain()
{
  // for viewport screen
  create_viewport_images();

  swap_chain_->set_render_pass(create_render_pass(), GUI_RENDER_PASS_ID);
  swap_chain_->set_frame_buffers(create_frame_buffers(), GUI_RENDER_PASS_ID);

  if (next_renderer_) next_renderer_->recreate_swap_chain();
}

VkRenderPass renderer::create_render_pass()
{
  VkAttachmentDescription attachment = {};
  attachment.format = swap_chain_->get_swap_chain_images_format();
  attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  // hve render pass's final layout
  attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_attachment = {};
  color_attachment.attachment = 0;
  color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_attachment;

  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // implies hve's render pass
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  info.attachmentCount = 1;
  info.pAttachments = &attachment;
  info.subpassCount = 1;
  info.pSubpasses = &subpass;
  info.dependencyCount = 1;
  info.pDependencies = &dependency;

  VkRenderPass render_pass;

  if (vkCreateRenderPass(device_.get_device(), &info, nullptr, &render_pass) != VK_SUCCESS)
    throw std::runtime_error("failed to create render pass.");

  return render_pass;
}

std::vector<VkFramebuffer> renderer::create_frame_buffers()
{
  // imgui frame buffer only takes image_resource view attachment
  VkImageView attachment;

  VkFramebufferCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  // make sure to create renderpass before frame buffers
  info.renderPass = swap_chain_->get_render_pass(GUI_RENDER_PASS_ID);
  info.attachmentCount = 1;
  info.pAttachments = &attachment;
  auto extent = swap_chain_->get_swap_chain_extent();
  info.width = extent.width;
  info.height = extent.height;
  info.layers = 1;

  // as many as image_resource view count
  auto get_image_count = swap_chain_->get_image_count();
  std::vector<VkFramebuffer> framebuffers(get_image_count);
  for (size_t i = 0; i < get_image_count; i++) {
    attachment = swap_chain_->get_image_view(i);
    if (vkCreateFramebuffer(device_.get_device(), &info, nullptr, &framebuffers[i]) != VK_SUCCESS)
      throw std::runtime_error("failed to create frame buffer.");
  }

  return framebuffers;
}

void renderer::create_viewport_images()
{
  auto image_count = swap_chain_->get_image_count();
  vp_images_.resize(image_count);

  for (uint32_t i = 0; i < image_count; i++) {
    vp_images_[i] = graphics::image_resource::create(
      device_,
      {swap_chain_->get_swap_chain_extent().width, swap_chain_->get_swap_chain_extent().height, 1},
      VK_FORMAT_B8G8R8A8_SRGB,
      VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
  }
}

std::vector<VkImageView> renderer::get_view_port_image_views() const
{
  std::vector<VkImageView> ret;
  for (auto& image : vp_images_) {
    ret.push_back(image->get_image_view());
  }
  return ret;
}
} // namespace hnll::gui