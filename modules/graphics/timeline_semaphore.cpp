// hnll
#include <graphics/timeline_semaphore.hpp>
#include <graphics/device.hpp>

namespace hnll::graphics {

u_ptr<timeline_semaphore> timeline_semaphore::create(device &device)
{ return std::make_unique<timeline_semaphore>(device); }

timeline_semaphore::timeline_semaphore(hnll::graphics::device &device) : device_(device)
{
  VkSemaphoreCreateInfo semaphore_info {
    VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
  };
  VkSemaphoreTypeCreateInfo semaphore_type_info {
    VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO
  };
  semaphore_type_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
  semaphore_info.pNext = &semaphore_type_info;
  vkCreateSemaphore(
    device_.get_device(),
    &semaphore_info,
    nullptr,
    &vk_timeline_semaphore_
  );
}

timeline_semaphore::~timeline_semaphore()
{ vkDestroySemaphore(device_.get_device(), vk_timeline_semaphore_, nullptr); }

void timeline_semaphore::wait_on_cpu()
{

}

void timeline_semaphore::wait_on_gpu()
{

}

} // namespace hnll::graphics