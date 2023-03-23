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

void physics_engine::submit_command()
{
  bool has_wait_semaphore = last_semaphore_value_ > 0;

  auto vk_semaphore = semaphore_->get_semaphore();
  // wait for previous frame's compute submission
  VkSemaphoreSubmitInfoKHR wait_semaphores[]{
    { VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR, nullptr, vk_semaphore, last_semaphore_value_, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR, 0 }
  };

  last_semaphore_value_++;

  VkSemaphoreSubmitInfoKHR signal_semaphores[]{
    { VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR, nullptr, vk_semaphore, last_semaphore_value_, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR, 0 },
  };

  VkCommandBufferSubmitInfoKHR command_buffer_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR };
  command_buffer_info.commandBuffer = command_buffer->vk_command_buffer;

  VkSubmitInfo2KHR submit_info{ VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR };
  submit_info.waitSemaphoreInfoCount = has_wait_semaphore ? 1 : 0;
  submit_info.pWaitSemaphoreInfos = wait_semaphores;
  submit_info.signalSemaphoreInfoCount = 1;
  submit_info.pSignalSemaphoreInfos = signal_semaphores;
  submit_info.commandBufferInfoCount = 1;
  submit_info.pCommandBufferInfos = &command_buffer_info;

  queue_submit2(compute_queue_, 1, &submit_info, VK_NULL_HANDLE);

  // submit the command buffer to the graphics queue with fence
  if (vkQueueSubmit(compute_queue_, 1, &submit_info, in_flight_fences_[current_frame_]) != VK_SUCCESS)
    throw std::runtime_error("failed to submit draw command buffer!");

}

} // namespace hnll::game