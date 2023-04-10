#pragma once

// hnll
#include <utils/common_alias.hpp>
#include <game/concepts.hpp>
#include <graphics/device.hpp>
#include <graphics/swap_chain.hpp>
#include <graphics/timeline_semaphore.hpp>

// std
#include <vector>

// std
#include <variant>
// lib
#include <vulkan/vulkan.h>

namespace hnll {

namespace game {

template <ComputeShader... C>
class compute_engine
{
    using shader_map = std::unordered_map<uint32_t, std::variant<u_ptr<C>...>>;
  public:
    static u_ptr<compute_engine<C...>> create(graphics::device& device, graphics::timeline_semaphore& semaphore)
    { return std::make_unique<compute_engine<C...>>(device, semaphore); }
    explicit compute_engine(graphics::device& device, graphics::timeline_semaphore& semaphore);
    ~compute_engine() { device_.free_command_buffers(std::move(command_buffers_), graphics::command_type::COMPUTE); }

    void render(float dt);

    // getter
    inline VkCommandBuffer get_current_command_buffer() const { return command_buffers_[current_frame_index_]; }

  private:
    void submit_command();

    template <ComputeShader Head, ComputeShader... Rest> void add_shader();
    void add_shader(){}

    graphics::device& device_;

    VkQueue compute_queue_;

    std::vector<VkCommandBuffer> command_buffers_;
    int current_frame_index_ = 0;
    bool is_frame_started_ = false;

    // ref to swap_chain::compute_semaphore
    graphics::timeline_semaphore& semaphore_;

    shader_map shaders_;
};

template <> class compute_engine<> {};

#define CMPT_ENGN_API  template<ComputeShader... CS>
#define CMPT_ENGN_TYPE compute_engine<CS...>

CMPT_ENGN_API CMPT_ENGN_TYPE::compute_engine(graphics::device &device, graphics::timeline_semaphore& semaphore)
 : device_(device), semaphore_(semaphore)
{
  command_buffers_ = device.create_command_buffers(graphics::swap_chain::MAX_FRAMES_IN_FLIGHT, graphics::command_type::COMPUTE);
  compute_queue_ = device.get_compute_queue();

  add_shader<CS...>();
}

CMPT_ENGN_API void CMPT_ENGN_TYPE::render(float dt)
{
  // begin command recording
  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  utils::compute_frame_info frame_info = {
      current_frame_index_,
      command_buffers_[current_frame_index_],
      dt
  };

  if (vkBeginCommandBuffer(command_buffers_[current_frame_index_], &begin_info) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }

  // visit all active shaders
  for (auto &shader : shaders_) {
    std::visit([&frame_info](auto &shader) { shader->render(frame_info); }, shader.second);
  }

  if (vkEndCommandBuffer(command_buffers_[current_frame_index_]) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }

  current_frame_index_ = ++current_frame_index_ == graphics::swap_chain::MAX_FRAMES_IN_FLIGHT
                           ? 0 : current_frame_index_;

  submit_command();
}

CMPT_ENGN_API void CMPT_ENGN_TYPE::submit_command()
{
  bool has_wait_semaphore = semaphore_.get_last_semaphore_value() > 0;

  auto vk_semaphore = semaphore_.get_vk_semaphore();
  // wait for previous frame's compute submission
  VkSemaphoreSubmitInfoKHR wait_semaphore { VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR };
  wait_semaphore.pNext = nullptr;
  wait_semaphore.semaphore = vk_semaphore;
  wait_semaphore.value = semaphore_.get_last_semaphore_value();
  wait_semaphore.stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR;
  wait_semaphore.deviceIndex = 0;

  semaphore_.increment_semaphore_value();

  VkSemaphoreSubmitInfoKHR signal_semaphore { VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR };
  signal_semaphore.pNext = nullptr;
  signal_semaphore.semaphore = vk_semaphore;
  signal_semaphore.value = semaphore_.get_last_semaphore_value();
  signal_semaphore.stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR;
  signal_semaphore.deviceIndex = 0;

  VkCommandBufferSubmitInfoKHR command_buffer_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR };
  command_buffer_info.commandBuffer = command_buffers_[current_frame_index_];

  VkSubmitInfo2KHR submit_info{ VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR };
  submit_info.waitSemaphoreInfoCount = has_wait_semaphore ? 1 : 0;
  submit_info.pWaitSemaphoreInfos = &wait_semaphore;
  submit_info.signalSemaphoreInfoCount = 1;
  submit_info.pSignalSemaphoreInfos = &signal_semaphore;
  submit_info.commandBufferInfoCount = 1;
  submit_info.pCommandBufferInfos = &command_buffer_info;

  // submit the command buffer to the graphics queue with fence
  if (vkQueueSubmit2(compute_queue_, 1, &submit_info, nullptr) != VK_SUCCESS)
    throw std::runtime_error("failed to submit draw command buffer!");
}

CMPT_ENGN_API template <ComputeShader Head, ComputeShader... Rest>
void CMPT_ENGN_TYPE::add_shader()
{
  auto system = Head::create(device_);
  static uint32_t shader_id = 0;
  shaders_[shader_id++] = std::move(system);

  if constexpr (sizeof...(Rest) >= 1)
    add_shader<Rest...>();
}

}} // namespace hnll::physics