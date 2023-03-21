// hnll
#include <game/modules/physics_engine.hpp>
#include <graphics/device.hpp>
#include <graphics/timeline_semaphore.hpp>
#include <graphics/swap_chain.hpp>

namespace hnll::game {

physics_engine::physics_engine(graphics::device &device) : device_(device)
{
  command_buffers_ = device.create_command_buffers(graphics::swap_chain::MAX_FRAMES_IN_FLIGHT, graphics::command_type::COMPUTE);
  compute_queue_ = device.get_compute_queue();
}

physics_engine::~physics_engine()
{
  device_.free_command_buffers(std::move(command_buffers_));
}

void physics_engine::submit_async_work()
{

}

void physics_engine::begin_command_recording()
{
  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(command_buffers_[current_command_index_], &begin_info) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }
}

void physics_engine::end_command_recording()
{
  if (vkEndCommandBuffer(command_buffers_[current_command_index_]) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }

  current_command_index_ = ++current_command_index_ == graphics::swap_chain::MAX_FRAMES_IN_FLIGHT
                           ? 0 : current_command_index_;
}

} // namespace hnll::game