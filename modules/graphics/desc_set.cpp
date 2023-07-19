// hnll
#include <graphics/device.hpp>
#include <graphics/buffer.hpp>
#include <graphics/desc_set.hpp>
#include <utils/singleton.hpp>

// std
#include <cassert>
#include <stdexcept>
#include <iostream>

namespace hnll::graphics {
// *************** Descriptor Set Layout builder *********************

desc_layout::builder::builder(hnll::graphics::device &device) : device_(device) {}

desc_layout::builder &desc_layout::builder::add_binding(
  VkDescriptorType descriptor_type,
  VkShaderStageFlags stage_flags,
  uint32_t count)
{
  VkDescriptorSetLayoutBinding layout_binding{};
  layout_binding.binding = bindings_.size();
  layout_binding.descriptorType = descriptor_type;
  layout_binding.descriptorCount = count;
  layout_binding.stageFlags = stage_flags;
  bindings_.emplace_back(layout_binding);
  return *this;
}

std::unique_ptr<desc_layout> desc_layout::builder::build()
{ return std::make_unique<desc_layout>(device_, std::move(bindings_)); }

// *************** Descriptor Set Layout *********************

u_ptr<desc_layout> desc_layout::create_from_bindings(device& device, const std::vector<binding_info>& bindings)
{
  builder build(device);
  for (const auto& binding : bindings) {
    build.add_binding(binding.desc_type, binding.shader_stages);
  }
  return build.build();
}

desc_layout::desc_layout
  (device &device, std::vector<VkDescriptorSetLayoutBinding>&& bindings) : device_{device}, bindings_{std::move(bindings)}
{

  VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info{};
  descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptor_set_layout_info.bindingCount = static_cast<uint32_t>(bindings_.size());
  descriptor_set_layout_info.pBindings = bindings_.data();

  if (vkCreateDescriptorSetLayout(device.get_device(), &descriptor_set_layout_info, nullptr, &descriptor_set_layout_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }
}

desc_layout::~desc_layout()
{ vkDestroyDescriptorSetLayout(device_.get_device(), descriptor_set_layout_, nullptr); }

// *************** Descriptor Pool builder *********************

desc_pool::builder::builder(hnll::graphics::device &device) : device_(device) {}

desc_pool::builder &desc_pool::builder::add_pool_size(VkDescriptorType descriptor_type, uint32_t count)
{
  pool_sizes_.push_back({descriptor_type, count});
  return *this;
}

desc_pool::builder &desc_pool::builder::set_pool_flags(VkDescriptorPoolCreateFlags flags)
{
  pool_flags_ = flags;
  return *this;
}
desc_pool::builder &desc_pool::builder::set_max_sets(uint32_t count)
{
  max_sets_ = count;
  return *this;
}

// build from its member
s_ptr<desc_pool> desc_pool::builder::build() const
{ return std::make_shared<desc_pool>(device_, max_sets_, pool_flags_, pool_sizes_); }

// *************** Descriptor Pool *********************

desc_pool::desc_pool(device &device, uint32_t max_sets, VkDescriptorPoolCreateFlags pool_flags,
                     const std::vector<VkDescriptorPoolSize> &pool_sizes) : device_{device}
{
  VkDescriptorPoolCreateInfo descriptor_pool_info{};
  descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptor_pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
  descriptor_pool_info.pPoolSizes = pool_sizes.data();
  descriptor_pool_info.maxSets = max_sets;
  descriptor_pool_info.flags = pool_flags | VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

  if (vkCreateDescriptorPool(device_.get_device(), &descriptor_pool_info, nullptr, &descriptor_pool_) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

desc_pool::~desc_pool() {
  vkDestroyDescriptorPool(device_.get_device(), descriptor_pool_, nullptr);
}

bool desc_pool::allocate_descriptor(const VkDescriptorSetLayout descriptor_set_layout, VkDescriptorSet &descriptor) const
{
  VkDescriptorSetAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc_info.descriptorPool = descriptor_pool_;
  alloc_info.pSetLayouts = &descriptor_set_layout;
  alloc_info.descriptorSetCount = 1;

  // Might want to create a "DescriptorPoolManager" class that handles this case, and builds
  // a new pool whenever an old pool fills up. But this is beyond our current scope
  if (vkAllocateDescriptorSets(device_.get_device(), &alloc_info, &descriptor) != VK_SUCCESS) {
    return false;
  }
  return true;
}

void desc_pool::free_descriptors(std::vector<VkDescriptorSet> &descriptors) const
{
  vkFreeDescriptorSets(
    device_.get_device(),
    descriptor_pool_,
    static_cast<uint32_t>(descriptors.size()),
    descriptors.data());
}

void desc_pool::reset_pool()
{ vkResetDescriptorPool(device_.get_device(), descriptor_pool_, 0); }

// *************** Descriptor Writer *********************

desc_writer::desc_writer(desc_layout &set_layout, desc_pool &pool)
  : set_layout_{set_layout}, pool_{pool} {}

desc_writer &desc_writer::write_buffer(uint32_t binding, VkDescriptorBufferInfo *buffer_info)
{
  auto &binding_description = set_layout_.bindings_[binding];

  assert(
    binding_description.descriptorCount == 1 &&
    "Binding single descriptor info, but binding expects multiple");

  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.descriptorType = binding_description.descriptorType;
  write.dstBinding = binding;
  write.pBufferInfo = buffer_info;
  write.descriptorCount = 1;

  writes_.push_back(write);
  return *this;
}

desc_writer &desc_writer::write_image(uint32_t binding, VkDescriptorImageInfo *image_info)
{
  auto &binding_description = set_layout_.bindings_[binding];

  assert(binding_description.descriptorCount == 1 &&
         "Binding single descriptor info, but binding expects multiple");

  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.descriptorType = binding_description.descriptorType;
  write.dstBinding = binding;
  write.pImageInfo = image_info;
  write.descriptorCount = 1;

  writes_.push_back(write);
  return *this;
}

desc_writer& desc_writer::write_acceleration_structure(uint32_t binding, void* as_info)
{
  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.pNext = as_info;
  write.dstBinding = binding;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

  writes_.push_back(write);
  return *this;
}

bool desc_writer::build(VkDescriptorSet &set)
{
  bool success = pool_.allocate_descriptor(set_layout_.get_descriptor_set_layout(), set);
  if (!success) {
    return false;
  }
  overwrite(set);
  return true;
}

void desc_writer::overwrite(VkDescriptorSet &set)
{
  for (auto &write : writes_) {
    write.dstSet = set;
  }
  vkUpdateDescriptorSets(pool_.device_.get_device(), writes_.size(), writes_.data(), 0, nullptr);
}

inline int calc_buffer_offset(int set, int binding, int frame)
{ return (set << 10) + (frame << 5) + binding; }

inline int calc_desc_set_offset(int set, int frame)
{ return (set << 5) + frame; }

// ************************* desc set ***********************************************************
desc_sets::desc_sets(
  device &device,
  const s_ptr<desc_pool> &pool,
  std::vector<desc_set_info>&& set_infos,
  int frame_count)
  : device_(device), pool_(pool)
{
  set_infos_ = std::move(set_infos);
  frame_count_ = frame_count;
  calc_resource_counts();
  build_layouts();
}

desc_sets::~desc_sets()
{
  pool_->free_descriptors(vk_desc_sets_);
}

void desc_sets::calc_resource_counts()
{
  // calc buffer count and desc sets count
  int buffer_count = 0;
  int desc_set_count = 0;

  for (int set = 0; set < set_infos_.size(); set++) {
    int resource_count = set_infos_[set].is_frame_buffered_ ? frame_count_ : 1;
    for (int frame = 0; frame < resource_count; frame++) {
      // update buffer count
      size_t binding_count = set_infos_[set].bindings_.size();
      for (int binding = 0; binding < binding_count; binding++) {
        int key = calc_buffer_offset(set, binding, frame);
        buffer_offset_dict_[key] = buffer_count++;
      }
      // update desc set count
      int key = calc_desc_set_offset(set, frame);
      desc_set_offset_dict_[key] = desc_set_count++;
    }
  }

  buffers_.resize(buffer_count);
  vk_desc_sets_.resize(desc_set_count);
}

void desc_sets::build_layouts()
{
  for (auto& set : set_infos_) {
    auto builder = desc_layout::builder(device_);
    for (auto& binding : set.bindings_) {
      builder.add_binding(binding.desc_type, binding.shader_stages);
    }
    layouts_.emplace_back(builder.build());
  }
}

void desc_sets::build()
{
  // build raw desc sets
  for (int set_id = 0; set_id < set_infos_.size(); set_id++) {
    int resource_count = set_infos_[set_id].is_frame_buffered_ ? frame_count_ : 1;
    for (int frame = 0; frame < resource_count; frame++) {
      // build for each frame
      auto& bindings = set_infos_[set_id].bindings_;
      auto writer = desc_writer(*layouts_[set_id], *pool_);
      for (int binding_id = 0; binding_id < bindings.size(); binding_id++) {
        auto buffer_info = get_buffer_r(set_id, binding_id, frame).desc_info();
        writer.write_buffer(binding_id, &buffer_info);
      }

      writer.build(get_vk_desc_set_r(set_id, frame));
    }
  }
}

// buffer update
void desc_sets::write_to_buffer(int set, int binding, int index, void *data)
{ get_buffer_r(set, binding, index).write_to_buffer(data); }

void desc_sets::flush_buffer(int set, int binding, int index)
{ get_buffer_r(set, binding, index).flush(); }

// getter
std::vector<VkDescriptorSetLayout> desc_sets::get_vk_layouts() const
{
  std::vector<VkDescriptorSetLayout> ret;
  for (auto& layout : layouts_) { ret.emplace_back(layout->get_descriptor_set_layout()); }
  return ret;
}

buffer& desc_sets::get_buffer_r(int set, int binding, int index)
{ return *buffers_[buffer_offset_dict_[calc_buffer_offset(set, binding, index)]]; }

VkDescriptorSet desc_sets::get_vk_desc_set(int set, int frame)
{ return vk_desc_sets_[desc_set_offset_dict_[calc_desc_set_offset(set, frame)]]; }

VkDescriptorSet& desc_sets::get_vk_desc_set_r(int set, int frame)
{ return vk_desc_sets_[desc_set_offset_dict_[calc_desc_set_offset(set, frame)]]; }

std::vector<VkDescriptorSet> desc_sets::get_vk_desc_sets(int frame)
{
  std::vector<VkDescriptorSet> ret;
  for (int set_id = 0; set_id < set_infos_.size(); set_id++) {
      ret.emplace_back(get_vk_desc_set(set_id, frame));
  }
  return ret;
}

// setter
void desc_sets::set_buffer(int set, int binding, int frame, u_ptr<buffer> &&desc_buffer)
{ buffers_[buffer_offset_dict_[calc_buffer_offset(set, binding, frame)]] = std::move(desc_buffer); }

} // namespace hnll::graphics