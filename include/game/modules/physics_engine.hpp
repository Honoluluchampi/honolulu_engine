#pragma once

// std
#include <vector>

// lib
#include <vulkan/vulkan.h>

namespace hnll {

namespace graphics { class device; }

namespace game {

class physics_engine
{
  public:
    explicit physics_engine(graphics::device& device);
    ~physics_engine();

    void submit_async_work();

    void begin_command_recording();
    void end_command_recording();

    VkCommandBuffer get_current_command_buffer() const { return command_buffers_[current_command_index_]; }

  private:
    graphics::device& device_;

    VkQueue compute_queue_;

    std::vector<VkCommandBuffer> command_buffers_;
    int current_command_index_ = 0;
    bool is_frame_started_ = false;

    int current_semaphore_value_ = 0;
};

}} // namespace hnll::physics