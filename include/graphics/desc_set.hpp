#pragma once

// hnll
#include <utils/common_alias.hpp>

// lib
#include <vulkan/vulkan.h>

//#include <graphics/device.hpp>

// std
#include <memory>
#include <unordered_map>
#include <vector>

namespace hnll::graphics {

class device;
class buffer;
class descriptor_pool;
class descriptor_set_layout;

class descriptor_set_layout {
  public:
    class builder {
      public:
        builder(device &device) : device_{device} {}

        builder &add_binding(uint32_t binding, VkDescriptorType descriptor_type, VkShaderStageFlags stage_flags, uint32_t count = 1);
        std::unique_ptr<descriptor_set_layout> build() const;

      private:
        device &device_;
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings_{};
    };

    // ctor dtor
    descriptor_set_layout(device &device, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
    ~descriptor_set_layout();
    descriptor_set_layout(const descriptor_set_layout &) = delete;
    descriptor_set_layout &operator=(const descriptor_set_layout &) = delete;

    VkDescriptorSetLayout get_descriptor_set_layout() const { return descriptor_set_layout_; }
    VkDescriptorSetLayout* get_p_descriptor_set_layout() { return &descriptor_set_layout_; }

  private:
    device &device_;
    VkDescriptorSetLayout descriptor_set_layout_;
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings_;

    friend class descriptor_writer;
};

class descriptor_pool {
  public:
    class builder {
      public:
        builder(device &device) : device_{device} {}

        builder &add_pool_size(VkDescriptorType descriptor_type, uint32_t count);
        builder &set_pool_flags(VkDescriptorPoolCreateFlags flags);
        builder &set_max_sets(uint32_t count);
        std::unique_ptr<descriptor_pool> build() const;

      private:
        device &device_;
        std::vector<VkDescriptorPoolSize> pool_sizes_{};
        uint32_t max_sets_ = 1000;
        VkDescriptorPoolCreateFlags pool_flags_ = 0;
    };

    descriptor_pool(
      device &device,
      uint32_t max_sets,
      VkDescriptorPoolCreateFlags pool_flags,
      const std::vector<VkDescriptorPoolSize> &pool_sizes);
    ~descriptor_pool();
    descriptor_pool(const descriptor_pool &) = delete;
    descriptor_pool &operator=(const descriptor_pool &) = delete;

    bool allocate_descriptor(const VkDescriptorSetLayout descriptor_set_layout, VkDescriptorSet &descriptor) const;
    void free_descriptors(std::vector<VkDescriptorSet> &descriptors) const;
    void reset_pool();

  private:
    device &device_;
    VkDescriptorPool descriptor_pool_;

    friend class descriptor_writer;
};

class descriptor_writer {
  public:
    descriptor_writer(descriptor_set_layout &set_layout, descriptor_pool &pool);

    descriptor_writer &write_buffer(uint32_t binding, VkDescriptorBufferInfo *buffer_info);
    descriptor_writer &write_image(uint32_t binding, VkDescriptorImageInfo *image_info);
    descriptor_writer &write_acceleration_structure(uint32_t binding, void* as_info);
    bool build(VkDescriptorSet &set);
    void overwrite(VkDescriptorSet &set);

  private:
    descriptor_set_layout &set_layout_;
    descriptor_pool &pool_;
    std::vector<VkWriteDescriptorSet> writes_;
};

class descriptor_set
{
  public:
    static u_ptr<descriptor_set> create(device& _device);

    descriptor_set(device& _device);
    ~descriptor_set();

    descriptor_set& create_pool(uint32_t max_sets, uint32_t desc_set_count, VkDescriptorType descriptor_type);
    descriptor_set& add_buffer(u_ptr<buffer>&& desc_buffer);
    descriptor_set& add_layout(VkShaderStageFlagBits shader_stage);
    descriptor_set& build_sets();

    // buffer update
    void write_to_buffer(size_t index, void *data);
    void flush_buffer(size_t index);

    // getter
    VkDescriptorSetLayout get_layout() const;
    VkDescriptorSet       get_set(size_t index) const { return sets_[index]; }

  private:
    device& device_;
    u_ptr<descriptor_pool>       pool_;
    u_ptr<descriptor_set_layout> layout_;
    std::vector<u_ptr<buffer>>   buffers_;
    std::vector<VkDescriptorSet> sets_;
    VkDescriptorType             type_;
};

} // namespace hnll::graphics