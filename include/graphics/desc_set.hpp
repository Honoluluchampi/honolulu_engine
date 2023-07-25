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

struct binding_info
{
  VkShaderStageFlags shader_stages;
  VkDescriptorType desc_type;
  std::string name;
};

// contains single set, multiple bindings
struct desc_set_info
{
  desc_set_info() {}
  desc_set_info(const std::vector<binding_info>& bindings, std::string set_name = "name")
  { bindings_ = bindings; name = set_name; }

  desc_set_info& add_binding(VkShaderStageFlags stage, VkDescriptorType type, std::string binding_name = "")
  {
    bindings_.emplace_back(stage, type, binding_name);
    return *this;
  }

  std::vector<binding_info> bindings_{};
  std::string name;
};

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

    static u_ptr<desc_layout> create_from_bindings(device& device, const std::vector<binding_info>& bindings);

    // ctor dtor
    desc_layout(device& device, std::vector<VkDescriptorSetLayoutBinding>&& bindings);
    desc_layout(device& device, const std::vector<binding_info>& bindings);
    ~desc_layout();

    VkDescriptorSetLayout get_descriptor_set_layout() const { return descriptor_set_layout_; }
    VkDescriptorSetLayout* get_p_descriptor_set_layout() { return &descriptor_set_layout_; }

  private:
    device &device_;
    VkDescriptorSetLayout descriptor_set_layout_;
    std::vector<VkDescriptorSetLayoutBinding> bindings_{};

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
        s_ptr<desc_pool> build() const;

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
    std::vector<VkWriteDescriptorSet> writes_{};
};

// contains multiple sets
struct desc_sets
{
  public:
    static u_ptr<desc_sets> create(
      device& device,
      const s_ptr<desc_pool>& pool,
      std::vector<desc_set_info>&& set_infos,
      int frame_count)
    { return std::make_unique<desc_sets>(device, pool, std::move(set_infos), frame_count); }

    desc_sets(
      device& device,
      const s_ptr<desc_pool>& pool,
      std::vector<desc_set_info>&& set_infos,
      int frame_count);

    ~desc_sets();

    // build sets after setting buffers
    void build();

    // getter
    std::vector<VkDescriptorSetLayout> get_vk_layouts() const;
    std::vector<VkDescriptorSet> get_vk_desc_sets(int frame);
    const std::vector<std::string>& get_buffer_debug_names() const { return buffer_debug_names_; }
    const std::vector<std::string>& get_vk_desc_sets_debug_names() const { return vk_desc_sets_debug_names_; }
    // for test
    int get_buffer_id(int frame, int set, int binding);
    int get_vk_desc_set_id(int frame, int set);

    // setter
    void set_buffer(int set, int binding, int frame, u_ptr<buffer>&& desc_buffer);

    // buffer update
    void write_to_buffer(int set, int binding, int frame, void *data);
    void flush_buffer(int set, int binding, int frame);

  private:
    void calc_resource_counts();
    void build_layouts();

    buffer& get_buffer_r(int set, int binding, int frame);
    VkDescriptorSet get_vk_desc_set(int set, int frame);
    VkDescriptorSet& get_vk_desc_set_r(int set, int frame);

    device& device_;
    s_ptr<desc_pool> pool_;
    std::vector<VkDescriptorSet> vk_desc_sets_{};
    std::vector<u_ptr<buffer>> buffers_{};
    std::vector<std::string> vk_desc_sets_debug_names_;
    std::vector<std::string> buffer_debug_names_;
    std::vector<u_ptr<desc_layout>> layouts_{};
    // buffer count offsets for each desc set
    std::unordered_map<int, int> buffer_offset_dict_{};
    std::unordered_map<int, int> desc_set_offset_dict_{};

    int frame_count_ = 1;

    // deleted after build
    std::vector<desc_set_info> set_infos_{};
};

} // namespace hnll::graphics