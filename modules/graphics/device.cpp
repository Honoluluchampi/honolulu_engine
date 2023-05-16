// hnll
#include <graphics/device.hpp>
#include <utils/rendering_utils.hpp>
#include <extensions/extensions_vk.hpp>

// ray tracing
#include <vulkan/vulkan_core.h>

// std headers
#include <cstring>
#include <iostream>
#include <set>
#include <unordered_set>

namespace hnll::graphics {

// local callback functions
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
  VkDebugUtilsMessageTypeFlagsEXT message_type,
  const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data,
  void *p_user_data)
{
  std::cerr << "validation layer: " << p_callback_data->pMessage << std::endl;

  return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(
  VkInstance instance,
  const VkDebugUtilsMessengerCreateInfoEXT *p_create_info,
  const VkAllocationCallbacks *p_allocator,
  VkDebugUtilsMessengerEXT *p_debug_messenger)
{
  // create the extension object if it's available
  // Since the debug messenger is specific to our Vulkan instance and its layers, it needs to be explicitly specified as first argument.
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
    instance,
    "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, p_create_info, p_allocator, p_debug_messenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugUtilsMessengerEXT(
  VkInstance instance,
  VkDebugUtilsMessengerEXT debug_messenger,
  const VkAllocationCallbacks *p_allocator)
{
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
    instance,
    "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debug_messenger, p_allocator);
  }
}

// class member functions
device::device(window &window, utils::rendering_type type)
  : window_{window}, rendering_type_(type)
{
  create_instance();
  // window surface should be created right after the instance creation,
  // because it can actually influence the physical device selection
  setup_debug_messenger();
  create_surface();
  pick_physical_device();
  setup_device_extensions(); // ray tracing
  create_logical_device(); // ray tracing
  graphics_command_pool_ = create_command_pool(command_type::GRAPHICS);
  compute_command_pool_  = create_command_pool(command_type::COMPUTE);
  transfer_command_pool_ = create_command_pool(command_type::TRANSFER);
}

device::~device()
{
  vkDestroyCommandPool(device_, graphics_command_pool_, nullptr);
  vkDestroyCommandPool(device_, compute_command_pool_, nullptr);
  vkDestroyCommandPool(device_, transfer_command_pool_, nullptr);
  // VkQueue is automatically destroyed when its device is deleted
  vkDestroyDevice(device_, nullptr);

  if (enable_validation_layers) {
    DestroyDebugUtilsMessengerEXT(instance_, debug_messenger_, nullptr);
  }

  vkDestroySurfaceKHR(instance_, surface_, nullptr);
  vkDestroyInstance(instance_, nullptr);
}

void device::setup_device_extensions()
{
  uint32_t extension_count = 0;
  vkEnumerateDeviceExtensionProperties(physical_device_, nullptr, &extension_count, nullptr);
  std::vector<VkExtensionProperties> extensions(extension_count);
  vkEnumerateDeviceExtensionProperties(physical_device_, nullptr, &extension_count, extensions.data());

  std::unordered_set<std::string> available;
  for (const auto &extension : extensions) {
    available.insert(extension.extensionName);
  }

  if (rendering_type_ == utils::rendering_type::RAY_TRACING) {
    device_extensions_ = {
      VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
      VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
      VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
      VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
      VK_KHR_MAINTENANCE_3_EXTENSION_NAME,
      VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
      VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    };
  }

  if (rendering_type_ == utils::rendering_type::MESH_SHADING) {
    device_extensions_ = {
      VK_NV_MESH_SHADER_EXTENSION_NAME,
      VK_NV_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME,
      VK_KHR_MAINTENANCE_4_EXTENSION_NAME,
    };
  }

  // common
  device_extensions_.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  device_extensions_.emplace_back(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
  device_extensions_.emplace_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
#ifndef __linux
    device_extensions_.emplace_back("VK_KHR_portability_subset");
#endif

  std::cout << "enabled device extensions:" << std::endl;
  auto& required_extensions = device_extensions_;
  for (const auto &required : required_extensions) {
    std::cout << "\t" << required << std::endl;
    if (available.find(required) == available.end()) {
      throw std::runtime_error("Missing required device extension");
    }
  }
}

// fill in a struct with some information about the application
void device::create_instance()
{
  // validation layers
  if (enable_validation_layers && !check_validation_layer_support()) {
    throw std::runtime_error("validation layers requested, but not available!");
  }

  VkApplicationInfo app_info = {};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "HonoluluVulkanEngine App";
  app_info.applicationVersion = VK_MAKE_VERSION(1, 3, 0);
  app_info.pEngineName = "No Engine";
  app_info.engineVersion = VK_MAKE_VERSION(1, 3, 0);
  app_info.apiVersion = VK_API_VERSION_1_3;

  VkInstanceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;

  // getting required extensions according to whether debug mode or not
  // glfw extensions are configured in somewhere else
  auto extensions = get_required_extensions();
  create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  create_info.ppEnabledExtensionNames = extensions.data();

  // additional debugger for vkCreateInstance and vkDestroyInstance
  VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info;
  if (enable_validation_layers) {
    create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers_.size());
    create_info.ppEnabledLayerNames = validation_layers_.data();

    populate_debug_messenger_create_info(debug_messenger_create_info);
    create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debug_messenger_create_info;
  }
  else {
    create_info.enabledLayerCount = 0;
    create_info.pNext = nullptr;
  }

  // 1st : pointer to struct with creation info
  // 2nd : pointer to custom allocator callbacks
  // 3rd : pointer to the variable that stores the handle to the new object
  if (vkCreateInstance(&create_info, nullptr, &instance_) != VK_SUCCESS)
    throw std::runtime_error("failed to create instance!");

  has_glfw_required_instance_extensions();
}

void device::pick_physical_device()
{
  // rate device suitability if its necessary
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);
  if (device_count == 0) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }
  // allocate an array to hold all of VkPhysicalDevice handle
  std::cout << "Device count: " << device_count << std::endl;
  std::vector<VkPhysicalDevice> devices(device_count);
  vkEnumeratePhysicalDevices(instance_, &device_count, devices.data());
  // physical_device_ is constructed as VK_NULL_HANDLE
  for (const auto &device : devices) {
    if (is_device_suitable(device)) {
      physical_device_= device;
      break;
    }
  }

  if (physical_device_ == VK_NULL_HANDLE) {
    throw std::runtime_error("failed to find a suitable GPU!");
  }

  vkGetPhysicalDeviceProperties(physical_device_, &properties);
  std::cout << "physical device: " << properties.deviceName << std::endl;
  swap_chain_support_details details;
  // surface capabilities
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device_, surface_, &details.capabilities_);
}

void device::create_logical_device()
{
  queue_family_indices indices = find_queue_families(physical_device_);

  // create a set of all unique queue families that are necessary for required queues
  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
  // if queue families are the same, handle for those queues are also same
  std::set<uint32_t> unique_queue_families = {
    indices.graphics_family_.value(),
    indices.present_family_.value(),
    indices.compute_family_.value(),
    indices.transfer_family_.value()
  };

  // if compute queue family is same as graphics, the priority for compute could be less than graphics
  float queue_property = 1.0f;
  for (uint32_t queue_family : unique_queue_families) {
    // VkDeviceQueueCreateInfo describes the number of queues we want for a single queue family
    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queue_family;
    queue_create_info.queueCount = 1;
    // Vulkan lets us assign priorities to queues to influence the scheduling of command buffer execution
    // using floating point numbers between 0.0 and 1.0
    queue_create_info.pQueuePriorities = &queue_property;
    queue_create_infos.push_back(queue_create_info);
  }

  // filling in the main VkDeviceCreateInfo structure;
  VkDeviceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
  create_info.pQueueCreateInfos = queue_create_infos.data();

  // common device features
  VkPhysicalDeviceFeatures device_features = {};
  device_features.samplerAnisotropy = VK_TRUE;

  VkPhysicalDeviceSynchronization2Features synchronization_2_features {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES
  };
  synchronization_2_features.synchronization2 = VK_TRUE;

  VkPhysicalDeviceTimelineSemaphoreFeatures timeline_semaphore_features {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES
  };
  timeline_semaphore_features.timelineSemaphore = VK_TRUE;
  timeline_semaphore_features.pNext = &synchronization_2_features;

  // configure device features for rasterize or ray tracing
  // for ray tracing
  if (rendering_type_ == utils::rendering_type::RAY_TRACING) {
    // enabling ray tracing extensions
    VkPhysicalDeviceBufferDeviceAddressFeaturesKHR enabled_buffer_device_addr {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
      nullptr
    };
    enabled_buffer_device_addr.bufferDeviceAddress = VK_TRUE;
    enabled_buffer_device_addr.pNext = &timeline_semaphore_features;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabled_ray_tracing_pipeline {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
      nullptr
    };
    enabled_ray_tracing_pipeline.rayTracingPipeline = VK_TRUE;
    enabled_ray_tracing_pipeline.pNext = &enabled_buffer_device_addr;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR enabled_acceleration_structure {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
      nullptr
    };
    enabled_acceleration_structure.accelerationStructure = VK_TRUE;
    enabled_acceleration_structure.pNext = &enabled_ray_tracing_pipeline;

    VkPhysicalDeviceDescriptorIndexingFeatures enabled_descriptor_indexing {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES
    };
    enabled_descriptor_indexing.pNext = &enabled_acceleration_structure;
    enabled_descriptor_indexing.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
    enabled_descriptor_indexing.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    enabled_descriptor_indexing.runtimeDescriptorArray = VK_TRUE;

    vkGetPhysicalDeviceFeatures(physical_device_, &device_features);
    VkPhysicalDeviceFeatures2 features2 {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, nullptr,
    };
    features2.pNext = &enabled_descriptor_indexing;
    features2.features = device_features;

    create_info.pNext = &features2;
    // device features are already included in features2
    create_info.pEnabledFeatures = nullptr;
  }

  // for mesh shader
  if (rendering_type_ == utils::rendering_type::MESH_SHADING) {
    // enable extensions
    // for uint8_t in taskNV
//    VkPhysicalDeviceVulkan12Features vulkan12_features {
//      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES
//    };
//    vulkan12_features.shaderInt8 = VK_TRUE;
//    vulkan12_features.pNext = &timeline_semaphore_features;

    VkPhysicalDeviceMaintenance4Features maintenance4_features {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES
    };
    maintenance4_features.maintenance4 = VK_TRUE;
    maintenance4_features.pNext = &timeline_semaphore_features;

    VkPhysicalDeviceMeshShaderFeaturesNV mesh_features = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV
    };
    mesh_features.meshShader = VK_TRUE;
    mesh_features.taskShader = VK_TRUE;
    mesh_features.pNext = &maintenance4_features;

    VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV baryFeatures = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_NV
    };
    baryFeatures.fragmentShaderBarycentric = VK_TRUE;
    baryFeatures.pNext = &mesh_features;

    vkGetPhysicalDeviceFeatures(physical_device_, &device_features);
    VkPhysicalDeviceFeatures2 features2 {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, nullptr
    };
    features2.pNext = &baryFeatures;
    features2.features = device_features;

    create_info.pNext = &features2;
    create_info.pEnabledFeatures = nullptr;
  }

  if (rendering_type_ == utils::rendering_type::VERTEX_SHADING) {
    VkPhysicalDeviceFeatures2 features2 {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, nullptr
    };
    features2.pNext = &timeline_semaphore_features;
    features2.features = device_features;

    create_info.pNext = &features2;
    create_info.pEnabledFeatures = nullptr;
  }

  // enable device extension
  create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions_.size());
  create_info.ppEnabledExtensionNames = device_extensions_.data();

  // might not really be necessary anymore because device specific validation layers
  // have been deprecated
  if (enable_validation_layers) {
    create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers_.size());
    create_info.ppEnabledLayerNames = validation_layers_.data();
  }
  else {
    create_info.enabledLayerCount = 0;
  }
  // instantiate the logical device
  // logical devices don't interact directly with  instances
  if (vkCreateDevice(physical_device_, &create_info, nullptr, &device_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create logical device!");
  }
  // retrieve queue handles for each queue family
  // simply use index 0, because were only creating a single queue from  this family
  vkGetDeviceQueue(device_, indices.graphics_family_.value(), 0, &graphics_queue_);
  vkGetDeviceQueue(device_, indices.present_family_.value(),  0, &present_queue_);
  vkGetDeviceQueue(device_, indices.compute_family_.value(),  0, &compute_queue_);
  vkGetDeviceQueue(device_, indices.transfer_family_.value(), 0, &transfer_queue_);

  if (indices.graphics_family_.value() == indices.compute_family_.value()) {
    std::cout << "same queue family for compute and graphics." << std::endl;
  }
  else {
    std::cout << "dedicated compute queue family detected." << std::endl;
  }

  if (rendering_type_ == utils::rendering_type::MESH_SHADING || rendering_type_ == utils::rendering_type::RAY_TRACING) {
    load_VK_EXTENSIONS(instance_, vkGetInstanceProcAddr, device_, vkGetDeviceProcAddr);
  }
}

// Command pools manage the memory that is used to store the buffers
// and command buffers are allocated from them.
VkCommandPool device::create_command_pool(command_type type)
{
  VkCommandPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.flags =
    VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  if (type == command_type::GRAPHICS)
    pool_info.queueFamilyIndex = queue_family_indices_.graphics_family_.value();
  if (type == command_type::COMPUTE)
    pool_info.queueFamilyIndex = queue_family_indices_.compute_family_.value();
  if (type == command_type::TRANSFER)
    pool_info.queueFamilyIndex = queue_family_indices_.transfer_family_.value();

  VkCommandPool pool;
  if (vkCreateCommandPool(device_, &pool_info, nullptr, &pool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
  }

  return pool;
}

void device::create_surface() { window_.create_window_surface(instance_, &surface_); }

// ensure there is at least one available physical device and
// the debice can present images to the surface we created
bool device::is_device_suitable(VkPhysicalDevice device)
{
  queue_family_indices indices = find_queue_families(device);

  bool extensions_supported = check_device_extension_support(device);

  bool swap_chain_adequate = false;
  if (extensions_supported) {
    swap_chain_support_details swap_chain_support = query_swap_chain_support(device);
    swap_chain_adequate = !swap_chain_support.formats_.empty() && !swap_chain_support.present_modes_.empty();
  }

  VkPhysicalDeviceFeatures supported_features;
  vkGetPhysicalDeviceFeatures(device, &supported_features);

  return indices.is_complete() && extensions_supported && swap_chain_adequate && supported_features.samplerAnisotropy;
}

void device::populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT &create_info)
{
  create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  create_info.pfnUserCallback = debugCallback;
  create_info.pUserData = nullptr;  // Optional
}

// fix this function to control debug call back of the apps
void device::setup_debug_messenger()
{
  if (!enable_validation_layers) return;
  VkDebugUtilsMessengerCreateInfoEXT create_info;
  populate_debug_messenger_create_info(create_info);
  if (CreateDebugUtilsMessengerEXT(instance_, &create_info, nullptr, &debug_messenger_) != VK_SUCCESS) {
    throw std::runtime_error("failed to set up debug messenger!");
  }
}

bool device::check_validation_layer_support()
{
  uint32_t layer_count;
  vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

  std::vector<VkLayerProperties> available_layers(layer_count);
  vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

  // check if all of the layers in validation_layers_ exist in the availableLyaerss
  for (const char *layer_name : validation_layers_) {
    bool layer_found = false;

    for (const auto &layer_properties : available_layers) {
      if (strcmp(layer_name, layer_properties.layerName) == 0) {
        layer_found = true;
        break;
      }
    }

    if (!layer_found) {
      return false;
    }
  }

  return true;
}

// required list of extensions based on whether validation layers are enabled
std::vector<const char *> device::get_required_extensions()
{
  uint32_t glfw_extension_count = 0;
  const char **glfw_extensions;
  glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

  // convert const char** to std::vector<const char*>
  std::vector<const char *> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

  if (enable_validation_layers) {
    extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  // ray tracing
  if (rendering_type_ != utils::rendering_type::VERTEX_SHADING) {
    extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
  }
  return extensions;
}

void device::has_glfw_required_instance_extensions()
{
  uint32_t extension_count = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
  std::vector<VkExtensionProperties> extensions(extension_count);
  vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

  std::unordered_set<std::string> available;
  for (const auto &extension : extensions) {
    available.insert(extension.extensionName);
  }

  std::cout << "enabled instance extensions:" << std::endl;
  auto required_extensions = get_required_extensions();
  for (const auto &required : required_extensions) {
    std::cout << "\t" << required << std::endl;
    if (available.find(required) == available.end()) {
      throw std::runtime_error("missing required glfw extension");
    }
  }
}

// check for swap chain extension
bool device::check_device_extension_support(VkPhysicalDevice device)
{
  uint32_t extension_count;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
  // each VkExtensionProperties contains the name and version of an extension
  std::vector<VkExtensionProperties> available_extensions(extension_count);
  vkEnumerateDeviceExtensionProperties(
    device,
    nullptr,
    &extension_count,
    available_extensions.data());

  std::set<std::string> required_extensions(device_extensions_.begin(), device_extensions_.end());
  for (const auto &extension : available_extensions) {
    required_extensions.erase(extension.extensionName);
  }

  return required_extensions.empty();
  // check whether all glfw_extensions are supported
}

queue_family_indices device::find_queue_families(VkPhysicalDevice device)
{
  queue_family_indices indices;
  // retrieve the list of quque families
  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
  // vkQueueFamilyProperties struct contains some details about the queue family
  // the type of operations, the number of queue that can be created based on that familyo
  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

  // check whether at least one queue_family support VK_QUEUE_GRAPHICS_BIT
  int i = 0;
  int compute_graphics_common_index;
  for (const auto &queue_family : queue_families) {
    // same i for presentFamily and graphicsFamily improves the performance
    // graphics: No DRI3 support detected - required for presentation
    // Note: you can probably enable DRI3 in your Xorg config
    if (queue_family.queueCount > 0 && queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphics_family_ = i;

      // prefer same queue for graphics and present
      VkBool32 present_support = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &present_support);
      if (queue_family.queueCount > 0 && present_support) {
        indices.present_family_ = i;
      }
    }

    // different queue for present from for graphics
    VkBool32 present_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &present_support);
    if (!indices.present_family_.has_value() && queue_family.queueCount > 0 && present_support) {
      indices.present_family_ = i;
    }

    // for async compute
    if (queue_family.queueCount > 0 && queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT) {
      // prefer different queue from graphics
      if (indices.graphics_family_ != i && !indices.compute_family_.has_value()) {
        indices.compute_family_ = i;
      }
      else
        compute_graphics_common_index = i;
    }

    // transfer queue
    if (queue_family.queueCount > 0 && queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT) {
      if (indices.transfer_family_ == std::nullopt) {
        indices.transfer_family_ = i;
      }
      // prefer different queue from graphics and compute
      if (!(queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) && !(queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
        indices.transfer_family_ = i;
      }
    }

    i++;
  }

  if (!indices.compute_family_.has_value()) {
    indices.compute_family_ = compute_graphics_common_index;
  }

  queue_family_indices_ = indices;

  return indices;
}

swap_chain_support_details device::query_swap_chain_support(VkPhysicalDevice device)
{
  swap_chain_support_details details;
  // surface capabilities
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities_);
  // surface format list
  uint32_t format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, nullptr);

  if (format_count != 0) {
    details.formats_.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, details.formats_.data());
  }

  uint32_t present_mode_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, nullptr);

  if (present_mode_count != 0) {
    details.present_modes_.resize(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
      device,
      surface_,
      &present_mode_count,
      details.present_modes_.data());
  }
  return details;
}

VkFormat device::find_supported_format(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
  for (VkFormat format : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(physical_device_, format, &props);

    if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
      return format;
    } else if (
      tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }
  throw std::runtime_error("failed to find supported format!");
}

uint32_t device::find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
  VkPhysicalDeviceMemoryProperties memory_properties;
  vkGetPhysicalDeviceMemoryProperties(physical_device_, &memory_properties);
  for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) &&
        (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}

void device::create_buffer(
  VkDeviceSize size,
  VkBufferUsageFlags usage,
  VkMemoryPropertyFlags properties,
  VkBuffer &buffer,
  VkDeviceMemory &buffer_memory)
{
  // buffer creation
  VkBufferCreateInfo buffer_info{};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = size;
  buffer_info.usage = usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device_, &buffer_info, nullptr, &buffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to create buffer!");
  }

  // memory allocation
  VkMemoryRequirements memory_requirements{};
  vkGetBufferMemoryRequirements(device_, buffer, &memory_requirements);

  VkMemoryAllocateInfo allocate_info{};
  allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocate_info.allocationSize = memory_requirements.size;
  allocate_info.memoryTypeIndex = find_memory_type(memory_requirements.memoryTypeBits, properties);

  // ray tracing (device address for buffer)
  if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
    VkMemoryAllocateFlagsInfo memory_allocate_flags_info {
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
      nullptr
    };
    memory_allocate_flags_info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
    if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
      allocate_info.pNext = &memory_allocate_flags_info;
    }
  }

  if (vkAllocateMemory(device_, &allocate_info, nullptr, &buffer_memory) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate buffer memory!");
  }
  // associate the memory with the buffer
  vkBindBufferMemory(device_, buffer, buffer_memory, 0);
}

VkCommandBuffer device::begin_one_shot_commands()
{
  VkCommandBufferAllocateInfo allocate_info{};
  allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocate_info.commandPool = transfer_command_pool_;
  allocate_info.commandBufferCount = 1;

  VkCommandBuffer command_buffer;
  vkAllocateCommandBuffers(device_, &allocate_info, &command_buffer);

  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(command_buffer, &begin_info);
  return command_buffer;
}

void device::end_one_shot_commands(VkCommandBuffer command_buffer)
{
  vkEndCommandBuffer(command_buffer);

  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer;

  vkQueueSubmit(transfer_queue_, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(transfer_queue_);

  vkFreeCommandBuffers(device_, transfer_command_pool_, 1, &command_buffer);
}

void device::copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size)
{
  // command buffer for memory transfer operations
  VkCommandBuffer command_buffer = begin_one_shot_commands();

  VkBufferCopy copy_region{};
  copy_region.srcOffset = 0;  // Optional
  copy_region.dstOffset = 0;  // Optional
  copy_region.size = size;
  vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

  end_one_shot_commands(command_buffer);
}

void device::copy_buffer_to_image(
  VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layer_count)
{
  VkCommandBuffer command_buffer = begin_one_shot_commands();

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = layer_count;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(
    command_buffer,
    buffer,
    image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &region);
  end_one_shot_commands(command_buffer);
}

std::vector<VkCommandBuffer> device::create_command_buffers(int count, command_type type)
{
  std::vector<VkCommandBuffer> command_buffers;
  command_buffers.resize(count);

  // specify command pool and number of buffers to allocate
  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  // if the allocated command buffers are primary or secondary command buffers
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = count;

  if (type == command_type::GRAPHICS)
    alloc_info.commandPool = graphics_command_pool_;
  if (type == command_type::COMPUTE)
    alloc_info.commandPool = compute_command_pool_;

  if (vkAllocateCommandBuffers(device_, &alloc_info, command_buffers.data()) != VK_SUCCESS)
    throw std::runtime_error("failed to allocate command buffers!");

  return command_buffers;
}

void device::free_command_buffers(std::vector<VkCommandBuffer> &&commands, command_type type)
{
  if (type == command_type::GRAPHICS) {
    vkFreeCommandBuffers(
      device_,
      graphics_command_pool_,
      static_cast<float>(commands.size()),
      commands.data());
  }
  if (type == command_type::COMPUTE) {
    vkFreeCommandBuffers(
      device_,
      compute_command_pool_,
      static_cast<float>(commands.size()),
      commands.data());
  }
  commands.clear();
}

void device::create_image_with_info(
  const VkImageCreateInfo &image_info,
  VkMemoryPropertyFlags properties,
  VkImage &image,
  VkDeviceMemory &image_memory)
{
  if (vkCreateImage(device_, &image_info, nullptr, &image) != VK_SUCCESS) {
    throw std::runtime_error("failed to create image!");
  }

  VkMemoryRequirements memory_requirements;
  vkGetImageMemoryRequirements(device_, image, &memory_requirements);

  VkMemoryAllocateInfo allocate_info{};
  allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocate_info.allocationSize = memory_requirements.size;
  allocate_info.memoryTypeIndex = find_memory_type(memory_requirements.memoryTypeBits, properties);

  if (vkAllocateMemory(device_, &allocate_info, nullptr, &image_memory) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate image memory!");
  }

  if (vkBindImageMemory(device_, image, image_memory, 0) != VK_SUCCESS) {
    throw std::runtime_error("failed to bind image memory!");
  }
}

VkShaderModule device::create_shader_module(const std::vector<char> &code)
{
  VkShaderModuleCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = code.size();
  // char to uint32_t
  create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

  VkShaderModule ret;
  if (vkCreateShaderModule(device_, &create_info, nullptr, &ret) != VK_SUCCESS)
    throw std::runtime_error("failed to create shader module!");

  return ret;
}

} // namespace hnll::graphics