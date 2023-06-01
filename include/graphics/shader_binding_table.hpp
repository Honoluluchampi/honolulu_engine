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
struct shader_entry
{
    static u_ptr<shader_entry> create(
      device& device,
      const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& properties,
      uint32_t handle_count);

    shader_entry(
      device& device,
      const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& properties,
      uint32_t handle_count);

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
      const std::vector<VkDescriptorSetLayout>& vk_desc_layouts,
      const std::vector<std::string>& shader_names,
      const std::vector<VkShaderStageFlagBits>& shader_stages);

    shader_binding_table(
      device& device,
      const std::vector<VkDescriptorSetLayout>& vk_desc_layouts,
      const std::vector<std::string>& shader_names,
      const std::vector<VkShaderStageFlagBits>& shader_stages);

  private:
    void setup_pipeline_properties();
    void setup_shader_groups(
      const std::vector<std::string>& shader_names,
      const std::vector<VkShaderStageFlagBits>& shader_stages);
    void setup_shader_entries();

    void create_pipeline(const std::vector<VkDescriptorSetLayout>& vk_desc_layouts);

    device& device_;
    std::vector<VkPipelineShaderStageCreateInfo> shader_stages_ {};
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shader_groups_ {};
    // actual shader binding table
    std::vector<u_ptr<shader_entry>> shader_entries_;
    // contains the count of each shader group
    std::array<uint32_t, 5> shader_counts_;
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR properties_ {};

    VkPipelineLayout pipeline_layout_;
    VkPipeline pipeline_;
};

} // namespace hnll::graphics