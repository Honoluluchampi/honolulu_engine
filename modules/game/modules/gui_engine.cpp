// hnll
#include <game/modules/gui_engine.hpp>
#include <gui/renderer.hpp>
#include <graphics/swap_chain.hpp>
#include <graphics/image_resource.hpp>

// embedded fonts
// download by yourself
#include <imgui/roboto_regular.embed>
#include <vulkan/vulkan.h>

namespace hnll::game {

ImVec2 gui_engine::viewport_size_;

// take s_ptr<swap_chain> from get_renderer
gui_engine::gui_engine(hnll::graphics::window& window, hnll::graphics::device& device)
  : device_(device)
{
  setup_specific_vulkan_objects();
  renderer_up_ = gui::renderer::create(window, device, false);

  setup_imgui(device, window.get_glfw_window());
  upload_font();

  setup_viewport();
}

gui_engine::~gui_engine()
{
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  cleanup_vulkan();
}

void gui_engine::setup_specific_vulkan_objects()
{
  create_descriptor_pool();
}

void gui_engine::setup_viewport()
{
  // setup sampler
  VkSamplerCreateInfo sampler_info{};
  sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  // filtering
  sampler_info.magFilter = VK_FILTER_LINEAR;
  sampler_info.minFilter = VK_FILTER_LINEAR;
  // matters when you sample outside the image
  // repeat is common for texture tiling
  sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

  sampler_info.anisotropyEnable = VK_FALSE;
  sampler_info.maxAnisotropy = 1.f;

  // when sampling beyond the image with clamp to border addressing mode
  sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

  sampler_info.unnormalizedCoordinates = VK_FALSE;

  // for percentage closer filtering on shadow maps
  sampler_info.compareEnable = VK_FALSE;
  sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;

  sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_info.mipLodBias = 0.0f;
  sampler_info.minLod = 0.0f;
  sampler_info.maxLod = 0.0f;

  if (vkCreateSampler(device_.get_device(), &sampler_info, nullptr, &viewport_sampler_) != VK_SUCCESS)
    throw std::runtime_error("failed to create sampler!");

  // set up viewport images
  const auto& vp_image_views = renderer_up_->get_view_port_image_views();
  viewport_image_ids_.resize(vp_image_views.size());
  for (int i = 0; i < vp_image_views.size(); i++) {
    viewport_image_ids_[i] = ImGui_ImplVulkan_AddTexture(
      viewport_sampler_,
      vp_image_views[i],
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }
}

void gui_engine::setup_imgui(hnll::graphics::device& device, GLFWwindow* window)
{
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO(); (void)io;
  // enable keyboard control
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  // setup ImGui style
  ImGui::StyleColorsDark();

  // setup glfw/graphics bindings
  ImGui_ImplGlfw_InitForVulkan(window, true);
  ImGui_ImplVulkan_InitInfo info = {};
  info.Instance = device.get_instance();
  info.PhysicalDevice = device.get_physical_device();
  info.Device = device_.get_device();
  // graphicsFamily's indice is needed (see device::create_command_pool)
  // but these are never used...
  info.QueueFamily = device.get_queue_family_indices().graphics_family_.value();
  graphics_queue_ = device.get_graphics_queue();
  info.Queue = graphics_queue_;
  info.PipelineCache = VK_NULL_HANDLE;
  info.DescriptorPool = descriptor_pool_;
  info.Allocator = nullptr;
  // TODO : make minImageCount consistent with hve
  info.MinImageCount = 2;
  info.ImageCount = renderer_up_->get_swap_chain_r().get_image_count();
  info.CheckVkResultFn = nullptr;

  // make sure to create render pass before this function
  ImGui_ImplVulkan_Init(&info, renderer_up_->get_render_pass());
}

void gui_engine::cleanup_vulkan()
{
  vkDestroySampler(device_.get_device(), viewport_sampler_, nullptr);
  vkDestroyDescriptorPool(device_.get_device(), descriptor_pool_, nullptr);
}

void gui_engine::begin_imgui()
{
  // start the imgui frame
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void gui_engine::render()
{
  // render viewport
  auto extent = renderer_up_->get_swap_chain_r().get_swap_chain_extent();
  float left_window_ratio = gui::renderer::get_left_window_ratio();
  float bottom_window_ratio = gui::renderer::get_bottom_window_ratio();

  ImGui::SetNextWindowPos(ImVec2(extent.width * left_window_ratio, 0));
  ImGui::SetNextWindowSize(ImVec2(
    extent.width * (1.f - left_window_ratio),
    extent.height * (1.f - bottom_window_ratio)));

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::Begin("viewport", nullptr, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize);

  ImVec2 panel_size(extent.width * (1 - left_window_ratio), extent.height * (1 - bottom_window_ratio));
  ImGui::Image(viewport_image_ids_[renderer_up_->get_frame_index()], panel_size);

  viewport_size_ = ImGui::GetWindowSize();

  ImGui::End();
  ImGui::PopStyleVar();

  // render stats
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(extent.width * left_window_ratio, extent.height));
  ImGui::Begin("stats", nullptr, ImGuiWindowFlags_NoResize);
  if (ImGui::TreeNode("window ratio")) {
    ImGui::SliderFloat("left window ratio", gui::renderer::get_left_window_ratio_p(), 0.f, 1.f);
    ImGui::SliderFloat("bottom window ratio", gui::renderer::get_bottom_window_ratio_p(), 0.f, 1.f);
    ImGui::TreePop();
  }
  ImGui::End();

  // render window
  ImGui::Render();

  frame_render();
}

void gui_engine::frame_render()
{
  // whether swap chain had been recreated
  if (renderer_up_->begin_frame()) {
    auto command_buffer = renderer_up_->begin_command_buffer(GUI_RENDER_PASS_ID);
    renderer_up_->begin_render_pass(command_buffer, GUI_RENDER_PASS_ID, renderer_up_->get_swap_chain_r().get_swap_chain_extent());

    // record the draw data to the command buffer
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);

    renderer_up_->end_render_pass(command_buffer);
    renderer_up_->end_frame(command_buffer);
  }
}

void gui_engine::create_descriptor_pool()
{
  // Create Descriptor Pool
  // TODO : understand these parameters
  VkDescriptorPoolSize pool_sizes[] =
    {
      { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
      { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
      { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
      { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
      { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
  VkDescriptorPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
  pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
  pool_info.pPoolSizes = pool_sizes;
  if (vkCreateDescriptorPool(device_.get_device(), &pool_info, nullptr, &descriptor_pool_) != VK_SUCCESS)
    throw std::runtime_error("failed to create descriptor pool.");
}

void gui_engine::upload_font()
{
  // load default font
  ImFontConfig font_config;
  font_config.FontDataOwnedByAtlas = false;
  ImGuiIO &io = ImGui::GetIO(); (void)io;
  ImFont* roboto_font = io.Fonts->AddFontFromMemoryTTF((void*)g_RobotoRegular, sizeof(g_RobotoRegular), 20.0f, &font_config);
  io.FontDefault = roboto_font;

  // upload fonts
  // Use any command queue
  VkCommandPool command_pool = renderer_up_->get_command_pool();
  VkCommandBuffer command_buffer = renderer_up_->get_current_command_buffer();
  if (vkResetCommandPool(device_.get_device(), command_pool, 0) != VK_SUCCESS)
    throw std::runtime_error("failed to reset command pool");

  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
    throw std::runtime_error("failed to begin command buffer.");

  ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

  VkSubmitInfo end_info = {};
  end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  end_info.commandBufferCount = 1;
  end_info.pCommandBuffers = &command_buffer;
  if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
    throw std::runtime_error("failed to end command buffer.");

  if (vkQueueSubmit(graphics_queue_, 1, &end_info, VK_NULL_HANDLE) != VK_SUCCESS)
    throw std::runtime_error("failed to submit font upload queue.");

  vkDeviceWaitIdle(device_.get_device());
  ImGui_ImplVulkan_DestroyFontUploadObjects();
}

graphics::renderer* gui_engine::renderer_p() const { return renderer_up_.get(); }

float gui_engine::get_left_window_ratio() { return gui::renderer::get_left_window_ratio(); }
float gui_engine::get_bottom_window_ratio() { return gui::renderer::get_bottom_window_ratio(); }

} // namespace hnll::gui