// hnll
#include <graphics/shader_binding_table.hpp>
#include <graphics/device.hpp>
#include <graphics/buffer.hpp>
#include <graphics/utils.hpp>

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

// core ------------------------------------------------------------------------------------
u_ptr<shader_binding_table> shader_binding_table::create(
  device &device,
  const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& properties,
  uint32_t handle_count)
{ return std::make_unique<shader_binding_table>(device, properties, handle_count); }

shader_binding_table::shader_binding_table(
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
}

} // namespace hnll::graphics