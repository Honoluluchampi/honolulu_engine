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

  private:
    VkCommandBuffer get_current_command_buffer()
    { return command_buffers_[current_frame_index_]; }

    void create_command_buffers();
    void free_command_buffers();

    graphics::device& device_;

    std::vector<VkCommandBuffer> command_buffers_;
    int current_frame_index_ = 0;
    bool is_frame_started_ = false;

    int current_semaphore_value_ = 0;
};

}} // namespace hnll::physics