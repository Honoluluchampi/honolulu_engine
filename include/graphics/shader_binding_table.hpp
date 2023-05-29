#pragma once

// hnll
#include <utils/common_alias.hpp>

// lib
#include <vulkan/vulkan.h>

namespace hnll::graphics {

class device;
class buffer;

class shader_binding_table
{
  public:
    static u_ptr<shader_binding_table> create(
      device& device,
      const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& properties,
      uint32_t handle_count);

    shader_binding_table(
      device& device,
      const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& properties,
      uint32_t handle_count);

  private:
    device& device_;
    u_ptr<graphics::buffer> buffer_;
    VkStridedDeviceAddressRegionKHR strided_device_address_region_;
};

class shader_binding_tables
{
  public:
  private:
};

} // namespace hnll::graphics