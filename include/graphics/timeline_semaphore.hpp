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

    // getter
    VkSemaphore     get_vk_semaphore() { return vk_timeline_semaphore_; }
    VkSemaphore*    get_vk_semaphore_r() { return &vk_timeline_semaphore_; }
    inline uint64_t get_last_semaphore_value() { return last_semaphore_value_; }

    inline bool has_work() const { return has_work_; }

    // setter
    inline void increment_semaphore_value() { last_semaphore_value_++; }
    inline void set_has_work(bool has_work) { has_work_ = has_work; }

  private:
    device& device_;
    VkSemaphore vk_timeline_semaphore_;
    uint64_t last_semaphore_value_ = 0;
    bool has_work_ = false;
};

} // namespace hnll::utils