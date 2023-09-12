#pragma once

// hnll
#include <utils/common_alias.hpp>
#include <utils/singleton.hpp>

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
      const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& properties,
      uint32_t handle_count);

    shader_entry(
      const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& properties,
      uint32_t handle_count);

    u_ptr<graphics::buffer> buffer_;
    VkStridedDeviceAddressRegionKHR strided_device_address_region_;
};

class shader_binding_table
{
  public:
    // names and shader_stages should correspond to each other
    static u_ptr<shader_binding_table> create(
      const std::vector<VkDescriptorSetLayout>& vk_desc_layouts,
      const std::vector<std::string>& shader_names,
      const std::vector<VkShaderStageFlagBits>& shader_stages);

    shader_binding_table(
      const std::vector<VkDescriptorSetLayout>& vk_desc_layouts,
      const std::vector<std::string>& shader_names,
      const std::vector<VkShaderStageFlagBits>& shader_stages);

    ~shader_binding_table();

    // getter
    inline VkPipeline get_pipeline() const { return pipeline_; }
    inline VkPipelineLayout get_pipeline_layout() const { return pipeline_layout_; }

    inline VkStridedDeviceAddressRegionKHR* get_gen_region_p() { return &gen_region_; }
    inline VkStridedDeviceAddressRegionKHR* get_miss_region_p() { return &miss_region_; }
    inline VkStridedDeviceAddressRegionKHR* get_hit_region_p() { return &hit_region_; }
    inline VkStridedDeviceAddressRegionKHR* get_callable_region_p() { return &callable_region_; }

  private:
    void setup_pipeline_properties();
    void setup_shader_groups(
      const std::vector<std::string>& shader_names,
      const std::vector<VkShaderStageFlagBits>& shader_stages);
    void create_pipeline(const std::vector<VkDescriptorSetLayout>& vk_desc_layouts);
    void setup_shader_entries();
    void copy_shader_address_region();

    utils::single_ptr<graphics::device> device_;
    std::vector<VkPipelineShaderStageCreateInfo> shader_stages_ {};
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shader_groups_ {};
    // actual shader binding table
    std::vector<u_ptr<shader_entry>> shader_entries_;
    // contains the count of each shader group
    std::array<uint32_t, 5> shader_counts_;
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR properties_ {};

    VkPipelineLayout pipeline_layout_;
    VkPipeline pipeline_;

    // actual shader device address region
    VkStridedDeviceAddressRegionKHR gen_region_ {};
    VkStridedDeviceAddressRegionKHR miss_region_ {};
    VkStridedDeviceAddressRegionKHR hit_region_ {};
    VkStridedDeviceAddressRegionKHR callable_region_ {};
};

} // namespace hnll::graphics