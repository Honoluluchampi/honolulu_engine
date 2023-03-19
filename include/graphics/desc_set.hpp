#pragma once

// hnll
#include <utils/common_alias.hpp>

// std
#include <unordered_map>

// lib
#include <vulkan/vulkan.h>

namespace hnll::graphics {

class device;
class buffer;

class desc_layout {
  public:
    class builder {
      public:
        builder(device &device);

        builder &add_binding(
          VkDescriptorType descriptor_type,
          VkShaderStageFlags stage_flags,
          uint32_t count = 1);
        u_ptr<desc_layout> build();

      private:
        device &device_;
        std::vector<VkDescriptorSetLayoutBinding> bindings_{};
    };

    // ctor dtor
    desc_layout(device &device, std::vector<VkDescriptorSetLayoutBinding>&& bindings);
    ~desc_layout();

    VkDescriptorSetLayout get_descriptor_set_layout() const { return descriptor_set_layout_; }
    VkDescriptorSetLayout* get_p_descriptor_set_layout() { return &descriptor_set_layout_; }

  private:
    device &device_;
    VkDescriptorSetLayout descriptor_set_layout_;
    std::vector<VkDescriptorSetLayoutBinding> bindings_;

    friend class desc_writer;
};

class desc_pool {
  public:
    class builder {
      public:
        explicit builder(device &device);

        builder &add_pool_size(VkDescriptorType descriptor_type, uint32_t count);
        builder &set_pool_flags(VkDescriptorPoolCreateFlags flags);
        builder &set_max_sets(uint32_t count);
        u_ptr<desc_pool> build() const;

      private:
        device &device_;
        std::vector<VkDescriptorPoolSize> pool_sizes_{};
        uint32_t max_sets_ = 1000;
        VkDescriptorPoolCreateFlags pool_flags_ = 0;
    };

    desc_pool(
      device &device,
      uint32_t max_sets,
      VkDescriptorPoolCreateFlags pool_flags,
      const std::vector<VkDescriptorPoolSize> &pool_sizes);
    ~desc_pool();

    bool allocate_descriptor(const VkDescriptorSetLayout descriptor_set_layout, VkDescriptorSet &descriptor) const;
    void free_descriptors(std::vector<VkDescriptorSet> &descriptors) const;
    void reset_pool();

  private:
    device &device_;
    VkDescriptorPool descriptor_pool_;

    friend class desc_writer;
};

class desc_writer {
  public:
    desc_writer(desc_layout &set_layout, desc_pool &pool);

    desc_writer &write_buffer(uint32_t binding, VkDescriptorBufferInfo *buffer_info);
    desc_writer &write_image(uint32_t binding, VkDescriptorImageInfo *image_info);
    desc_writer &write_acceleration_structure(uint32_t binding, void* as_info);
    bool build(VkDescriptorSet &set);
    void overwrite(VkDescriptorSet &set);

  private:
    desc_layout &set_layout_;
    desc_pool &pool_;
    std::vector<VkWriteDescriptorSet> writes_;
};

struct desc_binding
{
  desc_binding(VkDescriptorType type, int count)
  {
    desc_type = type;
    vk_desc_sets.resize(count);
    desc_buffers.resize(count);
  }
  std::vector<VkDescriptorSet> vk_desc_sets;
  std::vector<u_ptr<buffer>> desc_buffers;
  VkDescriptorType desc_type;
};



class desc_set
{
  public:
    static u_ptr<desc_set> create(device& _device);

    desc_set(device& _device);
    ~desc_set();

    desc_set& create_pool(uint32_t max_sets, uint32_t desc_set_count, VkDescriptorType descriptor_type);
    desc_set& add_buffer(u_ptr<buffer>&& desc_buffer);
    desc_set& add_layout(VkShaderStageFlagBits shader_stage);
    desc_set& build_sets();

    // buffer update
    void write_to_buffer(size_t index, void *data);
    void flush_buffer(size_t index);

    // getter
    VkDescriptorSetLayout get_layout() const;
    VkDescriptorSet       get_set(size_t binding, size_t index) const
    { return bindings_[binding].vk_desc_sets[index]; }

  private:
    device& device_;
    u_ptr<desc_layout> layout_;
    std::vector<desc_binding> bindings_;
};

} // namespace hnll::graphics