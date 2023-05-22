// hnll
#include <graphics/device.hpp>
#include <graphics/buffer.hpp>
#include <graphics/acceleration_structure.hpp>

namespace hnll {

VkDeviceAddress get_device_address(VkDevice device, VkBuffer buffer)
{
  VkBufferDeviceAddressInfo buffer_device_info {
    VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
    nullptr
  };
  buffer_device_info.buffer = buffer;
  return vkGetBufferDeviceAddress(device, &buffer_device_info);
}

namespace graphics {

acceleration_structure::acceleration_structure(hnll::graphics::device &device) : device_(device)
{

}

acceleration_structure::~acceleration_structure()
{
  vkDestroyAccelerationStructureKHR(device_.get_device(), as_handle_, nullptr);
}

void acceleration_structure::build_as(
  VkAccelerationStructureTypeKHR type,
  const hnll::graphics::acceleration_structure::input &input,
  VkBuildAccelerationStructureFlagsKHR build_flags)
{
  auto device = device_.get_device();

  // compute as size
  VkAccelerationStructureBuildGeometryInfoKHR geometry_info {
    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR
  };
  geometry_info.type = type;
  geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  geometry_info.flags = build_flags;
  geometry_info.geometryCount = static_cast<uint32_t>(input.geometry.size());
  geometry_info.pGeometries = input.geometry.data();

  VkAccelerationStructureBuildSizesInfoKHR size_info {
    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR
  };
  std::vector<uint32_t> primitives_count;
  primitives_count.reserve(input.build_range_info.size());
  for (int i = 0; i < input.build_range_info.size(); i++) {
    primitives_count.push_back(input.build_range_info[i].primitiveCount);
  }
  vkGetAccelerationStructureBuildSizesKHR(
    device,
    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
    &geometry_info,
    primitives_count.data(),
    &size_info
  );

  // create as
  VkBufferUsageFlags usage =
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
  VkMemoryPropertyFlags memory_props = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  // create buffer
  as_buffer_ = std::make_unique<buffer>(
    device_,
    size_info.accelerationStructureSize,
    1,
    usage,
    memory_props
  );
  as_size_ = size_info.accelerationStructureSize;
  VkAccelerationStructureCreateInfoKHR create_info  {
    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR
  };
  // create as object
  create_info.buffer = as_buffer_->get_buffer();
  create_info.size = as_size_;
  create_info.type = type;
  vkCreateAccelerationStructureKHR(
    device,
    &create_info,
    nullptr,
    &as_handle_
  );
  // get as device address
  VkAccelerationStructureDeviceAddressInfoKHR device_address_info {
    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR
  };
  device_address_info.accelerationStructure = as_handle_;
  as_device_address_ = hnll::get_device_address(device, as_buffer_->get_buffer());
  // create scratch buffer (for build)
  if (size_info.buildScratchSize > 0) {
    scratch_buffer_ = std::make_unique<buffer>(
      device_,
      size_info.buildScratchSize,
      1,
      usage,
      memory_props
    );
  }
  // create update buffer
//  if (size_info.updateScratchSize > 0) {
//    update_buffer_ = std::make_unique<buffer>(
//      device_,
//      size_info.updateScratchSize,
//      1,
//      usage,
//      memory_props
//    );
//  }
  // build as
  geometry_info.dstAccelerationStructure = as_handle_;
  geometry_info.scratchData.deviceAddress = hnll::get_device_address(device, scratch_buffer_->get_buffer());
  build(geometry_info, input.build_range_info);
}

void acceleration_structure::build(
  const VkAccelerationStructureBuildGeometryInfoKHR &build_geometry_info,
  const std::vector<VkAccelerationStructureBuildRangeInfoKHR> &build_range_info)
{
  std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> build_range_info_ptr;
  for (auto& info : build_range_info) {
    build_range_info_ptr.push_back(&info);
  }
  auto command = device_.begin_one_shot_commands();
  vkCmdBuildAccelerationStructuresKHR(
    command,
    1,
    &build_geometry_info,
    build_range_info_ptr.data()
  );
  // create barrier
  VkMemoryBarrier barrier { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
  barrier.srcAccessMask =
    VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR |
    VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
  barrier.dstAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;
  vkCmdPipelineBarrier(
    command,
    VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
    VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
    0,
    1,
    &barrier,
    0,
    nullptr,
    0,
    nullptr
  );
  // wait for building...
  device_.end_one_shot_commands(command);
}

void acceleration_structure::destroy_scratch_buffer()
{
  if (auto buffer = scratch_buffer_->get_buffer(); buffer) {
    vkDestroyBuffer(device_.get_device(), buffer, nullptr);
  }
}

}} // namespace hnll::graphics