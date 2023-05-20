#pragma once

// hnll
#include <utils/common_alias.hpp>

// lib
#include <vulkan/vulkan.h>

namespace hnll {

VkDeviceAddress get_device_address(VkDevice device, VkBuffer buffer);

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
    [[nodiscard]] VkDeviceAddress            get_device_address() const { return as_device_address_; }

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