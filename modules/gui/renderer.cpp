// hnll
#include <gui/renderer.hpp>
#include <graphics/image_resource.hpp>
#include <graphics/desc_set.hpp>
#include <utils/rendering_utils.hpp>

namespace hnll::gui {

float renderer::left_window_ratio_ = 0.2f;
float renderer::bottom_window_ratio_ = 0.25f;

u_ptr<renderer> renderer::create(
  graphics::window& window,
  graphics::device& device,
  utils::rendering_type type,
  bool recreate_from_scratch)
{ return std::make_unique<renderer>(window, device, type, recreate_from_scratch); }

renderer::renderer(
  graphics::window& window,
  graphics::device& device,
  utils::rendering_type type,
  bool recreate_from_scratch) :
  hnll::graphics::renderer(window, device, recreate_from_scratch)
{
  rendering_type_ = type;
  recreate_swap_chain();
}

void renderer::recreate_swap_chain()
{
  // for viewport screen
  create_viewport_images();

  swap_chain_->set_render_pass(create_viewport_render_pass(), VIEWPORT_RENDER_PASS_ID);
  swap_chain_->set_frame_buffers(create_viewport_frame_buffers(), VIEWPORT_RENDER_PASS_ID);
  swap_chain_->set_render_pass(create_imgui_render_pass(), GUI_RENDER_PASS_ID);
  swap_chain_->set_frame_buffers(create_imgui_frame_buffers(), GUI_RENDER_PASS_ID);

  if (next_renderer_) next_renderer_->recreate_swap_chain();
}

VkRenderPass renderer::create_viewport_render_pass()
{
  VkAttachmentDescription color = {};
  color.format = swap_chain_->get_swap_chain_images_format();
  color.samples = VK_SAMPLE_COUNT_1_BIT;
  color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkAttachmentDescription depth = {};
  // same as graphics::swap_chain
  depth.format = device_.find_supported_format(
    {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
    VK_IMAGE_TILING_OPTIMAL,
    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

  depth.samples = VK_SAMPLE_COUNT_1_BIT;
  depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  std::array<VkAttachmentDescription, 2> attachments = { color, depth };

  VkAttachmentReference color_ref = {};
  color_ref.attachment = 0;
  color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depth_ref = {};
  depth_ref.attachment = 1;
  depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_ref;
  subpass.pDepthStencilAttachment = &depth_ref;
  subpass.inputAttachmentCount = 0;
  subpass.pInputAttachments = nullptr;
  subpass.preserveAttachmentCount = 0;
  subpass.pPreserveAttachments = nullptr;
  subpass.pResolveAttachments = nullptr;

  std::array<VkSubpassDependency, 2> dependencies;
  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL; // implies hve's render pass
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

  VkRenderPassCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  info.attachmentCount = static_cast<uint32_t>(attachments.size());
  info.pAttachments = attachments.data();
  info.subpassCount = 1;
  info.pSubpasses = &subpass;
  info.dependencyCount = static_cast<uint32_t>(dependencies.size());
  info.pDependencies = dependencies.data();

  VkRenderPass render_pass;

  if (vkCreateRenderPass(device_.get_device(), &info, nullptr, &render_pass) != VK_SUCCESS)
    throw std::runtime_error("failed to create render pass.");

  return render_pass;
}

VkRenderPass renderer::create_imgui_render_pass()
{
  VkAttachmentDescription color = {};
  color.format = swap_chain_->get_swap_chain_images_format();
  color.samples = VK_SAMPLE_COUNT_1_BIT;
  color.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_ref = {};
  color_ref.attachment = 0;
  color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_ref;

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
  info.pAttachments = &color;
  info.subpassCount = 1;
  info.pSubpasses = &subpass;
  info.dependencyCount = 1;
  info.pDependencies = &dependency;

  VkRenderPass render_pass;

  if (vkCreateRenderPass(device_.get_device(), &info, nullptr, &render_pass) != VK_SUCCESS)
    throw std::runtime_error("failed to create render pass.");

  return render_pass;
}

std::vector<VkFramebuffer> renderer::create_viewport_frame_buffers()
{
  auto image_count = vp_images_.size();
  std::vector<VkFramebuffer> frame_buffers(image_count);

  for (size_t i = 0; i < image_count; i++) {
    std::array<VkImageView, 2> attachments = {
      vp_images_[i]->get_image_view(),
      swap_chain_->get_depth_image_view(i)
    };

    VkFramebufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    // make sure to create render pass before frame buffers
    info.renderPass = swap_chain_->get_render_pass(VIEWPORT_RENDER_PASS_ID);
    info.attachmentCount = static_cast<uint32_t>(attachments.size());
    info.pAttachments = attachments.data();
    auto extent = swap_chain_->get_swap_chain_extent();
    info.width  = static_cast<uint32_t>(extent.width * (1 - left_window_ratio_));
    info.height = static_cast<uint32_t>(extent.height * (1 - bottom_window_ratio_));
    info.layers = 1;

    if (vkCreateFramebuffer(device_.get_device(), &info, nullptr, &frame_buffers[i]) != VK_SUCCESS)
      throw std::runtime_error("failed to create frame buffer.");
  }
  return frame_buffers;
}

std::vector<VkFramebuffer> renderer::create_imgui_frame_buffers()
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

  auto extent = swap_chain_->get_swap_chain_extent();

  if (rendering_type_ != utils::rendering_type::RAY_TRACING) {
    for (uint32_t i = 0; i < image_count; i++) {
      vp_images_[i] = graphics::image_resource::create(
        device_,
        {static_cast<uint32_t>(extent.width * (1 - left_window_ratio_)),
         static_cast<uint32_t>(extent.height * (1 - bottom_window_ratio_)), 1},
        VK_FORMAT_B8G8R8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
      );
    }
  }
  // for ray tracing
  else {
    // create image resource
    for (uint32_t i = 0; i < image_count; ++i) {
      // this image will be used as desc_image by ray tracing shader
      vp_images_[i] = graphics::image_resource::create(
        device_,
        {static_cast<uint32_t>(extent.width * (1 - left_window_ratio_)),
         static_cast<uint32_t>(extent.height * (1 - bottom_window_ratio_)), 1},
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        true // for_ray_tracing
      );
    }

    // create desc sets from images
    desc_layout_ = graphics::desc_layout::builder(device_)
      .add_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR) // ray traced image
      .build();

    vk_desc_layout_ = desc_layout_->get_descriptor_set_layout();

    desc_pool_ = graphics::desc_pool::builder(device_)
      .set_max_sets(10)
      .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10)
      .set_pool_flags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
      .build();

    vp_image_descs_.resize(image_count);

    for (uint32_t i = 0; i < image_count; ++i) {
      VkDescriptorImageInfo image_info {
        .imageView = vp_images_[i]->get_image_view(),
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL
      };

      graphics::desc_writer(*desc_layout_, *desc_pool_)
        .write_image(0, &image_info)
        .build(vp_image_descs_[i]);
    }
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

void renderer::transition_vp_image_layout(int frame_index, VkImageLayout new_layout, VkCommandBuffer command)
{ vp_images_[frame_index]->transition_image_layout(new_layout, command); }

} // namespace hnll::gui