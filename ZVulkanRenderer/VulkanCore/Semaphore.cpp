#include "Semaphore.h"
#include <stdexcept>

namespace VKFW::vulkancore
{

Semaphore::Semaphore(const std::shared_ptr<Device> &device) : mDevice(device)
{
    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkResult result = vkCreateSemaphore(mDevice->getDevice(), &semaphoreCreateInfo, nullptr, &mSemaphore);
    if (result != VK_SUCCESS)
        throw std::runtime_error("failed to create semaphore");
}

Semaphore::~Semaphore()
{
    if (mSemaphore != VK_NULL_HANDLE)
        vkDestroySemaphore(mDevice->getDevice(), mSemaphore, nullptr);
}

} // namespace VKFW::vulkancore