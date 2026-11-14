#include "Fence.h"
#include <stdexcept>

namespace VKFW::vulkancore
{

Fence::Fence(const std::shared_ptr<Device> &device, bool signaled) : mDevice(device)
{
    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

    VkResult result = vkCreateFence(mDevice->getDevice(), &fenceCreateInfo, nullptr, &mFence);
    if (result != VK_SUCCESS)
        throw std::runtime_error("failed to create fence");
}

Fence::~Fence()
{
    if (mFence != VK_NULL_HANDLE)
        vkDestroyFence(mDevice->getDevice(), mFence, nullptr);
}

void Fence::waitForFence(uint64_t timeout)
{
    vkWaitForFences(mDevice->getDevice(), 1, &mFence, VK_TRUE, timeout);
}

void Fence::resetFence()
{
    vkResetFences(mDevice->getDevice(), 1, &mFence);
}

} // namespace VKFW::vulkancore