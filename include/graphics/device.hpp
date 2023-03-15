#pragma once

// hnll
#include <utils/common_alias.hpp>
#include <graphics/window.hpp>

// lib
#include <vulkan/vulkan.hpp>

// std
#include <string>
#include <vector>
#include <optional>

namespace hnll {

// forward declaration
namespace utils { enum class rendering_type; }

namespace graphics {

// non-member function
struct swap_chain_support_details
{
  VkSurfaceCapabilitiesKHR        capabilities_;
  std::vector<VkSurfaceFormatKHR> formats_;
  std::vector<VkPresentModeKHR>   present_modes_;
};

struct queue_family_indices
{
  std::optional<uint32_t> graphics_family_ = std::nullopt;
  std::optional<uint32_t> present_family_  = std::nullopt;
  std::optional<uint32_t> compute_family_  = std::nullopt;
  inline bool is_complete() { return
    (graphics_family_ != std::nullopt) &&
    (present_family_  != std::nullopt) &&
    (compute_family_  != std::nullopt); };
};

class device
{
  public:
#ifdef NDEBUG
    const bool enable_validation_layers = false;
#else
    const bool enable_validation_layers = true;
#endif

    device(window &window, utils::rendering_type type);
    ~device();

    // Not copyable or movable
    device(const device &) = delete;
    device& operator=(const device &) = delete;
    device(device &&) = delete;
    device &operator=(device &&) = delete;

    static u_ptr<device> create(window &window, utils::rendering_type type)
    { return std::make_unique<device>(window, type); }

    // getter
    VkCommandPool         get_command_pool()         { return command_pool_; }
    VkInstance            get_instance()             { return instance_; }
    VkPhysicalDevice      get_physical_device()      { return physical_device_; }
    VkDevice              get_device()               { return device_; }
    VkSurfaceKHR          get_surface()              { return surface_; }
    VkQueue               get_graphics_queue()       { return graphics_queue_; }
    VkQueue               get_present_queue()        { return present_queue_; }
    queue_family_indices  get_queue_family_indices() { return queue_family_indices_; }
    utils::rendering_type get_rendering_type() const { return rendering_type_; }

    swap_chain_support_details get_swap_chain_support() { return query_swap_chain_support(physical_device_); }
    uint32_t                   find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties);
    queue_family_indices       find_physical_queue_families() { return find_queue_families(physical_device_); }
    VkFormat                   find_supported_format(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

    // Buffer Helper Functions
    void create_buffer(
      VkDeviceSize size,
      VkBufferUsageFlags usage,
      VkMemoryPropertyFlags properties,
      VkBuffer &buffer,
      VkDeviceMemory &buffer_memory);
    VkCommandBuffer begin_one_shot_commands();
    void end_one_shot_commands(VkCommandBuffer command_buffer);
    void copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);
    void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layer_count);

    void create_image_with_info(
      const VkImageCreateInfo &image_info,
      VkMemoryPropertyFlags properties,
      VkImage &image,
      VkDeviceMemory &image_memory);

    VkPhysicalDeviceProperties properties;

  private:
    // create sequence
    void setup_device_extensions();
    void create_instance();
    void setup_debug_messenger();
    void create_surface();
    void pick_physical_device();
    void create_logical_device();
    void create_command_pool();

    // helper functions
    bool is_device_suitable(VkPhysicalDevice device);
    std::vector<const char *> get_required_extensions();
    bool check_validation_layer_support();
    queue_family_indices find_queue_families(VkPhysicalDevice device);
    void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT &create_info);
    void has_glfw_required_instance_extensions();
    // check for swap chain extension
    bool check_device_extension_support(VkPhysicalDevice device);
    swap_chain_support_details query_swap_chain_support(VkPhysicalDevice device);

    // hnll
    window& window_;
    // vulkan
    VkDevice device_;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkInstance instance_;
    VkSurfaceKHR surface_;
    VkCommandPool command_pool_;
    VkQueue graphics_queue_;
    VkQueue present_queue_;
    VkQueue compute_queue_;
    queue_family_indices queue_family_indices_; // for hie ctor
    VkDebugUtilsMessengerEXT debug_messenger_;

    const std::vector<const char *> validation_layers_ = {"VK_LAYER_KHRONOS_validation"};
    std::vector<const char *> device_extensions_;

    utils::rendering_type rendering_type_;
};


}} // namespace hnll::graphics