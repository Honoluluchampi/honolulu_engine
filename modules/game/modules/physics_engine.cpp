// hnll
#include <game/modules/physics_engine.hpp>
#include <graphics/device.hpp>
#include <graphics/timeline_semaphore.hpp>
#include <graphics/swap_chain.hpp>

namespace hnll::game {

physics_engine::physics_engine(graphics::device &device) : device_(device)
{
  create_command_buffers();
}

physics_engine::~physics_engine()
{
  free_command_buffers();
}

void physics_engine::submit_async_work()
{

}

void physics_engine::create_command_buffers()
{
  command_buffers_.resize(graphics::swap_chain::MAX_FRAMES_IN_FLIGHT);

  // specify command pool and number of buffers to allocate
  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  // if the allocated command buffers are primary or secondary command buffers
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandPool = device_.get_command_pool();
  alloc_info.commandBufferCount = static_cast<uint32_t>(command_buffers_.size());

  if (vkAllocateCommandBuffers(device_.get_device(), &alloc_info, command_buffers_.data()) != VK_SUCCESS)
    throw std::runtime_error("failed to allocate command buffers!");
}

void physics_engine::free_command_buffers()
{
  vkFreeCommandBuffers(
    device_.get_device(),
    device_.get_command_pool(),
    static_cast<float>(command_buffers_.size()),
    command_buffers_.data());
  command_buffers_.clear();
}

} // namespace hnll::game