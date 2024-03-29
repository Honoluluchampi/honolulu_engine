#pragma once

// hnll
#include <utils/common_alias.hpp>

// lib
#include <vulkan/vulkan.h>

namespace hnll {
namespace graphics {

// forward declaration
class device;
class buffer;

class acceleration_structure
{
  public:
    struct input {
      std::vector<VkAccelerationStructureGeometryKHR> geometry;
      std::vector<VkAccelerationStructureBuildRangeInfoKHR> build_range_info;
    };

    static u_ptr<acceleration_structure> create(device& device);

    explicit acceleration_structure(device& device);
    ~acceleration_structure();

    void build_as(
      VkAccelerationStructureTypeKHR type,
      const input& input,
      VkBuildAccelerationStructureFlagsKHR build_flags
    );

    void destroy_scratch_buffer();

    // getter
    [[nodiscard]] VkAccelerationStructureKHR get_as_handle()      const { return as_handle_; }
    [[nodiscard]] VkDeviceAddress            get_as_device_address() const { return as_device_address_; }
    // for descriptor writing
    [[nodiscard]] VkWriteDescriptorSetAccelerationStructureKHR get_as_info() const
    {
      return {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
        .accelerationStructureCount = 1,
        .pAccelerationStructures = &as_handle_,
      };
    }

  private:
    void build(
      const VkAccelerationStructureBuildGeometryInfoKHR& build_geometry_info,
      const std::vector<VkAccelerationStructureBuildRangeInfoKHR>& build_range_info
    );

    //--------- variables ---------------------------------------------
    device& device_;

    // as resources
    VkAccelerationStructureKHR as_handle_ = VK_NULL_HANDLE;
    VkDeviceAddress            as_device_address_ = 0;
    VkDeviceSize               as_size_ = 0;
    u_ptr<graphics::buffer>    as_buffer_;

    // helper buffers
    u_ptr<graphics::buffer> scratch_buffer_;
//    u_ptr<graphics::buffer> update_buffer_;
};

}} // namespace hnll::graphics