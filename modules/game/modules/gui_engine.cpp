// hnll
#include <game/modules/gui_engine.hpp>
#include <gui/renderer.hpp>
#include <graphics/swap_chain.hpp>

// embedded fonts
// download by yourself
#include <imgui/roboto_regular.embed>

namespace hnll::game {

// take s_ptr<swap_chain> from get_renderer
gui_engine::gui_engine(hnll::graphics::window& window, hnll::graphics::device& device)
  : device_(device.get_device())
{
  setup_specific_vulkan_objects();
  renderer_up_ = gui::renderer::create(window, device, false);
  setup_imgui(device, window.get_glfw_window());
  upload_font();
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

void gui_engine::setup_imgui(hnll::graphics::device& device, GLFWwindow* window)
{
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO(); (void)io;
  // enable keyboard control
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  // setup ImGui style
  ImGui::StyleColorsDark();

  // setup glfw/graphics bindings
  ImGui_ImplGlfw_InitForVulkan(window, true);
  ImGui_ImplVulkan_InitInfo info = {};
  info.Instance = device.get_instance();
  info.PhysicalDevice = device.get_physical_device();
  device_ = device.get_device();
  info.Device = device_;
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
  vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr);
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

  // render window
  ImGui::Render();

  frame_render();
}

void gui_engine::frame_render()
{
  // whether swap chain had been recreated
  if (auto command_buffer = renderer_up_->begin_frame()) {
    renderer_up_->begin_swap_chain_render_pass(command_buffer, GUI_RENDER_PASS_ID);

    // record the draw data to the command buffer
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);

    renderer_up_->end_swap_chain_render_pass(command_buffer);
    renderer_up_->end_frame();
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
  if (vkCreateDescriptorPool(device_, &pool_info, nullptr, &descriptor_pool_) != VK_SUCCESS)
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
  if (vkResetCommandPool(device_, command_pool, 0) != VK_SUCCESS)
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

  vkDeviceWaitIdle(device_);
  ImGui_ImplVulkan_DestroyFontUploadObjects();
}

graphics::renderer* gui_engine::renderer_p() const { return renderer_up_.get(); }

} // namespace hnll::gui