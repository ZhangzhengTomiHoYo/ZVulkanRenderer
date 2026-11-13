#include "CommandPool.h"
#include <stdexcept>

namespace VKFW::vulkancore
{

CommandPool::CommandPool(const std::shared_ptr<Device> &device, VkCommandPoolCreateFlagBits flags) : mDevice(device)
{
    VkCommandPoolCreateInfo commandPoolCreateInfo{};
    commandPoolCreateInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.queueFamilyIndex = mDevice->getGraphicQueueFamily().value();   // 创建池时需要指定队列 后续的command buffer不能跨队列族提交
    commandPoolCreateInfo.flags            = flags;

    VkResult result = vkCreateCommandPool(mDevice->getDevice(), &commandPoolCreateInfo, nullptr, &mCommandPool);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create command pool");
    }
}

CommandPool::~CommandPool()
{
    if (mCommandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(mDevice->getDevice(), mCommandPool, nullptr);
    }
}

} // namespace VKFW::vulkancore
