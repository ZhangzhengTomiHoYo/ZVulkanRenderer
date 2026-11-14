#pragma once
#include "Device.h"
#include <memory>

namespace VKFW::vulkancore
{

class Fence
{
  public:
    Fence(const std::shared_ptr<Device> &device, bool signaled = true);
    ~Fence();

    void waitForFence(uint64_t timeout = UINT64_MAX);
    void resetFence();

    VkFence getFence() const { return mFence; }

  private:
    VkFence                 mFence  = VK_NULL_HANDLE;
    std::shared_ptr<Device> mDevice = nullptr;
};

} // namespace VKFW::vulkancore