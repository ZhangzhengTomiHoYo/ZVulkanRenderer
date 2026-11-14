#pragma once
#include "Device.h"
#include <memory>

namespace VKFW::vulkancore
{

class Semaphore
{
  public:
    Semaphore(const std::shared_ptr<Device> &device);
    ~Semaphore();

    VkSemaphore getSemaphore() const { return mSemaphore; }

  private:
    VkSemaphore             mSemaphore = VK_NULL_HANDLE;
    std::shared_ptr<Device> mDevice    = nullptr;
};

} // namespace VKFW::vulkancore