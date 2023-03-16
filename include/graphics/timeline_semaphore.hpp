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

  private:
    device& device_;
    VkSemaphore vk_timeline_semaphore_;
};

} // namespace hnll::utils