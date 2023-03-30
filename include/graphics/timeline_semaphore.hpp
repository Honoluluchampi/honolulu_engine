#pragma once

// hnll
#include <utils/common_alias.hpp>

// lib
#include <vulkan/vulkan.h>

namespace hnll::graphics {

class device;

class timeline_semaphore
{
  public:
    static u_ptr<timeline_semaphore> create(device& device);

    explicit timeline_semaphore(device& device);
    ~timeline_semaphore();

    void wait_on_cpu();
    void wait_on_gpu();

    // getter
    VkSemaphore get_semaphore() { return vk_timeline_semaphore_; }
    inline uint64_t get_last_semaphore_value() { return last_semaphore_value_; }

    // setter
    inline void increment_semaphore_value() { last_semaphore_value_++; }
  private:
    device& device_;
    VkSemaphore vk_timeline_semaphore_;
    uint64_t last_semaphore_value_ = 0;
};

} // namespace hnll::utils