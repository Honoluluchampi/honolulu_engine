#pragma once

// hnll
#include <utils/common_alias.hpp>

// std
#include <memory>

// lib
#include <vulkan/vulkan.h>

namespace hnll{

namespace graphics {

class device;

class buffer {
  public:
    buffer(
      device& device,
      VkDeviceSize instance_size,
      uint32_t instance_count,
      VkBufferUsageFlags usage_flags,
      VkMemoryPropertyFlags memory_property_flags,
      VkDeviceSize min_offset_alignment = 1);
    ~buffer();

    static u_ptr<buffer> create(
      device& device,
      VkDeviceSize instance_size,
      uint32_t instance_count,
      VkBufferUsageFlags usage_flags,
      VkMemoryPropertyFlags memory_property_flags,
      void* writing_data,
      VkDeviceSize min_offset_alignment = 1);

    static u_ptr<buffer> create_with_staging(
      device& device,
      VkDeviceSize instance_size,
      uint32_t instance_count,
      VkBufferUsageFlags usage_flags,
      VkMemoryPropertyFlags memory_property_flags,
      void* writing_data,
      VkDeviceSize min_offset_alignment = 1);

    buffer(const buffer&) = delete;
    buffer& operator=(const buffer&) = delete;

    VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    void unmap();

    void write_to_buffer(void* data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

    VkDescriptorBufferInfo desc_info(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;
    VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

    // for multiple instances per buffer
    void write_to_index(void* data, int index);
    VkResult flush_index(int index);
    VkDescriptorBufferInfo desc_info_for_index(int index) const;
    VkResult invalidate_index(int index);

    VkBuffer              get_buffer()                const { return buffer_; }
    void*                 get_mapped_memory()         const { return mapped_; }
    uint32_t              get_instance_count()        const { return instance_count_; }
    VkDeviceSize          get_instance_size()         const { return instance_size_; }
    VkDeviceSize          get_alignment_size()        const { return instance_size_; }
    VkBufferUsageFlags    get_usage_flags()           const { return usage_flags_; }
    VkMemoryPropertyFlags get_memory_property_flags() const { return memory_property_flags_; }
    VkDeviceSize          get_buffer_size()           const { return buffer_size_; }

  private:
    static VkDeviceSize get_alignment(VkDeviceSize instance_size, VkDeviceSize min_offset_alignment);

    device& device_;
    void* mapped_ = nullptr;

    // separating buffer and its assigned memory enables programmer to manage memory manually
    VkBuffer buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;

    VkDeviceSize          buffer_size_;
    uint32_t              instance_count_;
    VkDeviceSize          instance_size_;
    VkDeviceSize          alignment_size_;
    VkBufferUsageFlags    usage_flags_;
    VkMemoryPropertyFlags memory_property_flags_;
};

}} // namespace hnll::graphics