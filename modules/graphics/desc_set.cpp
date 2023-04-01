// hnll
#include <graphics/device.hpp>
#include <graphics/buffer.hpp>
#include <graphics/desc_set.hpp>

// std
#include <cassert>
#include <stdexcept>

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
  descriptor_pool_info.flags = pool_flags;

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


// ************************* desc set ***********************************************************
desc_sets::desc_sets(device &device, const s_ptr<desc_pool> &pool, std::vector<desc_set_info> set_infos)
  : device_(device), pool_(pool)
{
  size_t set_count = set_infos.size();
  vk_desc_sets_.resize(set_count);
  buffers_.resize(set_count);
  layouts_.resize(set_count);


}

desc_sets::~desc_sets()
{ pool_->free_descriptors(vk_desc_sets_); }
//
//desc_set& desc_set::create_pool(
//  uint32_t max_sets, uint32_t desc_set_count, VkDescriptorType descriptor_type)
//{
//  pool_ = desc_pool::builder(device_)
//    .set_max_sets(max_sets)
//    .add_pool_size(descriptor_type, desc_set_count)
//    .set_pool_flags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
//    .build();
//
//  return *this;
//}
//
//desc_set& desc_set::add_binding(
//  VkShaderStageFlags shader_stages,
//  VkDescriptorType desc_type,
//  size_t buffer_count)
//{ bindings_.push_back(desc_binding(shader_stages, desc_type, buffer_count)); }
//
//desc_set& desc_set::set_buffer(size_t binding, size_t index, u_ptr<buffer>&& desc_buffer)
//{ bindings_[binding].desc_buffers[index] = std::move(desc_buffer); return *this; }
//
//desc_set& desc_set::build_sets()
//{
//  // build layout
//  auto builder = desc_layout::builder(device_);
//  for (auto& binding : bindings_) {
//    builder.add_binding(binding.desc_type, binding.shader_stages);
//  }
//  layout_ = builder.build();
//
//  // build raw desc sets
//  for (auto& binding : bindings_) {
//    auto set_count = binding.desc_buffers.size();
//
//    for (int i = 0; i < set_count; i++) {
//      auto buffer_info = binding.desc_buffers[i]->desc_info();
//      desc_writer(*layout_, *pool_)
//        .write_buffer(i, &buffer_info)
//        .build(binding.vk_desc_sets[i]);
//    }
//  }
//  return *this;
//}
//
//// buffer update
//void desc_set::write_to_buffer(size_t binding, size_t index, void *data)
//{ bindings_[binding].desc_buffers[index]->write_to_buffer(data); }
//
//void desc_set::flush_buffer(size_t binding, size_t index)
//{ bindings_[binding].desc_buffers[index]->flush(); }
//
//// getter
//VkDescriptorSetLayout desc_set::get_layout() const
//{ return layout_->get_descriptor_set_layout(); }

} // namespace hnll::graphics