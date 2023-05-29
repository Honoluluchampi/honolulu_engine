#pragma once

// hnll
#include <utils/common_alias.hpp>

// lib
#include <vulkan/vulkan.h>

namespace hnll::graphics {

class device;
class buffer;

enum class rt_shader_id : uint32_t {
    GEN,  // ray generation
    MISS, // miss
    CHIT, // closest-hit
    AHIT, // any-hit
    INT,  // intersection
};

// each entry is able to have multiple same type shaders
class shader_entry
{
  public:
    static u_ptr<shader_entry> create(
      device& device,
      const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& properties,
      uint32_t handle_count);

    shader_entry(
      device& device,
      const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& properties,
      uint32_t handle_count);

  private:
    device& device_;
    u_ptr<graphics::buffer> buffer_;
    VkStridedDeviceAddressRegionKHR strided_device_address_region_;
};

class shader_binding_table
{
  public:
    // names and shader_stages should correspond to each other
    u_ptr<shader_binding_table> create(
      device& device,
      const std::vector<std::string>& shader_names,
      const std::vector<VkShaderStageFlagBits>& shader_stages);

    shader_binding_table(
      device& device,
      const std::vector<std::string>& shader_names,
      const std::vector<VkShaderStageFlagBits>& shader_stages);

  private:
    void setup_shader_groups(
      const std::vector<std::string>& shader_names,
      const std::vector<VkShaderStageFlagBits>& shader_stages);
    void setup_shader_entries(
      const std::vector<std::string>& shader_names,
      const std::vector<VkShaderStageFlagBits>& shader_stages);

    device& device_;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shader_groups_;
    std::vector<u_ptr<shader_entry>> shader_entries_;
};

} // namespace hnll::graphics