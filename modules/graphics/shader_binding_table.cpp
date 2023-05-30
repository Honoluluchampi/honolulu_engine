// hnll
#include <graphics/shader_binding_table.hpp>
#include <graphics/device.hpp>
#include <graphics/buffer.hpp>
#include <graphics/utils.hpp>
#include <utils/utils.hpp>

namespace hnll::graphics {

// utils functions ------------------------------------------------------------------------
template<class T> T align(T size, uint32_t align)
{ return (size + align - 1) & ~static_cast<T>(align - 1); }

VkStridedDeviceAddressRegionKHR get_sbt_entry_strided_device_address_region(
  VkDevice device,
  VkBuffer buffer,
  const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& properties,
  uint32_t handle_count)
{
  const uint32_t handle_size_aligned = align(properties.shaderGroupHandleSize, properties.shaderGroupHandleAlignment);
  VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegionKHR{};
  stridedDeviceAddressRegionKHR.deviceAddress = get_device_address(device, buffer);
  stridedDeviceAddressRegionKHR.stride = handle_size_aligned;
  stridedDeviceAddressRegionKHR.size = handle_count * handle_size_aligned;
  return stridedDeviceAddressRegionKHR;
}

VkPipelineShaderStageCreateInfo load_shader(
  VkDevice device,
  const std::string& file_path,
  VkShaderStageFlagBits stage)
{
  // read spv
  auto shader_spv = utils::read_file_for_shader(file_path);
  VkShaderModuleCreateInfo module_create_info {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .pNext = nullptr,
    .codeSize = uint32_t(shader_spv.size()),
    .pCode = reinterpret_cast<const uint32_t*>(shader_spv.data())
  };

  // create shader module
  VkShaderModule shader_module;
  vkCreateShaderModule(device, &module_create_info, nullptr, &shader_module);

  VkPipelineShaderStageCreateInfo shader_create_info {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .pNext = nullptr,
    .stage = stage,
    .module = shader_module,
    .pName = "main"
  };

  return shader_create_info;
}

// shader entry ------------------------------------------------------------------------------------
u_ptr<shader_entry> shader_entry::create(
  device &device,
  const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& properties,
  uint32_t handle_count)
{ return std::make_unique<shader_entry>(device, properties, handle_count); }

shader_entry::shader_entry(
  device &device,
  const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& properties,
  uint32_t handle_count)
 : device_(device)
{
  buffer_ = buffer::create(
    device,
    properties.shaderGroupHandleSize * handle_count,
    1,
    VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    nullptr
  );
  strided_device_address_region_ = get_sbt_entry_strided_device_address_region(
    device_.get_device(),
    buffer_->get_buffer(),
    properties,
    handle_count
  );
  buffer_->map();
}

// shader binding table -----------------------------------------------------------------------------
u_ptr<shader_binding_table> shader_binding_table::create(
  device &device,
  const std::vector<std::string> &shader_names,
  const std::vector<VkShaderStageFlagBits> &shader_stages)
{ return std::make_unique<shader_binding_table>(device, shader_names, shader_stages); }

shader_binding_table::shader_binding_table(
  device &device,
  const std::vector<std::string> &shader_names,
  const std::vector<VkShaderStageFlagBits> &shader_stages)
  : device_(device)
{
  assert(shader_names.size() == shader_stages.size() && "the size of shader_names and shader_stages should be matched.");

  setup_pipeline_properties();
  setup_shader_groups(shader_names, shader_stages);
  setup_shader_entries();
}

void shader_binding_table::setup_pipeline_properties()
{
  properties_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
  VkPhysicalDeviceProperties2 deviceProperties2{};
  deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
  deviceProperties2.pNext = &properties_;
  vkGetPhysicalDeviceProperties2(device_.get_physical_device(), &deviceProperties2);
}

void shader_binding_table::setup_shader_groups(
  const std::vector<std::string> &shader_names,
  const std::vector<VkShaderStageFlagBits> &shader_stages)
{
  // poll shader types
  std::array<std::vector<std::string>, 5> shader_types_polled;
  for (int i = 0; i < shader_names.size(); i++) {
    switch (shader_stages[i]) {
      case VK_SHADER_STAGE_RAYGEN_BIT_KHR :
        shader_types_polled[static_cast<uint32_t>(rt_shader_id::GEN)].push_back(shader_names[i]);
        break;

      case VK_SHADER_STAGE_MISS_BIT_KHR :
        shader_types_polled[static_cast<uint32_t>(rt_shader_id::MISS)].push_back(shader_names[i]);
        break;

      case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR :
        shader_types_polled[static_cast<uint32_t>(rt_shader_id::CHIT)].push_back(shader_names[i]);
        break;

      case VK_SHADER_STAGE_ANY_HIT_BIT_KHR :
        shader_types_polled[static_cast<uint32_t>(rt_shader_id::AHIT)].push_back(shader_names[i]);
        break;

      case VK_SHADER_STAGE_INTERSECTION_BIT_KHR :
        shader_types_polled[static_cast<uint32_t>(rt_shader_id::INT)].push_back(shader_names[i]);
        break;
      default :
        std::cerr << "not ray tracing shader : " + shader_names[i] << std::endl;
        break;
    }
  }

  for (int i = 0; i < 5; i++) {
    shader_counts_[i] = shader_types_polled[i].size();
  }

  // actual shader modules
  std::vector<VkPipelineShaderStageCreateInfo> stages{};

  // build
  for (int type = 0; type < 5; type++) {
    // if the group has any entries
    if (!shader_types_polled[type].empty()) {
      VkRayTracingShaderGroupCreateInfoKHR shader_group{};
      shader_group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
      // gen
      if (type == 0) {
        shader_group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        shader_group.closestHitShader   = VK_SHADER_UNUSED_KHR;
        shader_group.anyHitShader       = VK_SHADER_UNUSED_KHR;
        shader_group.intersectionShader = VK_SHADER_UNUSED_KHR;
        for (const auto& name : shader_types_polled[type]) {
          auto stage = load_shader(device_.get_device(), name, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
          stages.push_back(stage);
          shader_group.generalShader = static_cast<uint32_t>(stages.size()) - 1;
        }
        shader_groups_.push_back(shader_group);
      }
      // miss
      if (type == 0) {
        shader_group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        shader_group.closestHitShader   = VK_SHADER_UNUSED_KHR;
        shader_group.anyHitShader       = VK_SHADER_UNUSED_KHR;
        shader_group.intersectionShader = VK_SHADER_UNUSED_KHR;
        for (const auto& name : shader_types_polled[type]) {
          auto stage = load_shader(device_.get_device(), name, VK_SHADER_STAGE_MISS_BIT_KHR);
          stages.push_back(stage);
          shader_group.generalShader = static_cast<uint32_t>(stages.size()) - 1;
        }
        shader_groups_.push_back(shader_group);
      }
        // chit
      else if (type == 2) {
        shader_group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
        shader_group.generalShader      = VK_SHADER_UNUSED_KHR;
        shader_group.anyHitShader       = VK_SHADER_UNUSED_KHR;
        shader_group.intersectionShader = VK_SHADER_UNUSED_KHR;
        for (const auto& name : shader_types_polled[type]) {
          auto stage = load_shader(device_.get_device(), name, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
          stages.push_back(stage);
          shader_group.closestHitShader = static_cast<uint32_t>(stages.size()) - 1;
        }
        shader_groups_.push_back(shader_group);
      }
      else {
        throw std::runtime_error("Unsupported ray tracing shader");
      }
    }
  }
}

void shader_binding_table::setup_shader_entries()
{
  uint32_t handle_size = properties_.shaderGroupHandleSize;
  uint32_t handle_size_aligned = align(handle_size, properties_.shaderGroupHandleAlignment);
  uint32_t group_count = static_cast<uint32_t>(shader_groups_.size());
  uint32_t sbt_size = group_count * handle_size_aligned;

  std::vector<uint8_t> shader_handle_storage(sbt_size);
  vkGetRayTracingShaderGroupHandlesKHR(
    device_.get_device(),
    pipeline_,
    0,
    group_count,
    sbt_size,
    shader_handle_storage.data()
  );

  uint32_t current_handle_count = 0;
  shader_entries_.resize(shader_groups_.size());
  for (int i = 0; i < 5; i++) {
    if (shader_counts_[i] > 0) {
      shader_entries_[i] = shader_entry::create(device_, properties_, shader_counts_[i]);
      shader_entries_[i]->buffer_->write_to_buffer(
        shader_handle_storage.data() + handle_size_aligned * current_handle_count,
        handle_size * shader_counts_[i]);
      current_handle_count += shader_counts_[i];
    }
  }
}

} // namespace hnll::graphics