#pragma once
#include "Device.h"
#include <memory>

namespace VKFW::vulkancore
{

class CommandPool
{
  public:
    CommandPool(const std::shared_ptr<Device> &device, VkCommandPoolCreateFlagBits = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    ~CommandPool();
    VkCommandPool getCommandPool() const { return mCommandPool; }

  private:
    VkCommandPool           mCommandPool = VK_NULL_HANDLE;
    std::shared_ptr<Device> mDevice      = nullptr;
};

} // namespace VKFW::vulkancore
