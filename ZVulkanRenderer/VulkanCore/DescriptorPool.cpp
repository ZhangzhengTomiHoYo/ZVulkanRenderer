#include "DescriptorPool.h"
#include <stdexcept>

namespace VKFW::vulkancore
{

DescriptorPool::DescriptorPool(const std::shared_ptr<Device> &device) : mDevice(device) {}

DescriptorPool::~DescriptorPool()
{
    destroyPool();
}

void DescriptorPool::build(const std::vector<std::shared_ptr<DescriptorParameter>> &parameters, int frameCount)
{
    destroyPool();

    // 统计 UBO 描述符数量
    uint32_t uboCount = 0;
    for (const auto &parameter : parameters)
        if (parameter->mDescriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            uboCount += parameter->mCount;

    // TODO：统计组合图像采样器描述符数量
    uint32_t samplerCount = 0;
    for (const auto &p : parameters)
        if (p->mDescriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
            samplerCount += p->mCount;

    std::vector<VkDescriptorPoolSize> poolSizes;
    if (uboCount > 0)
    {
        VkDescriptorPoolSize poolSize{};
        poolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = uboCount * frameCount;
        poolSizes.push_back(poolSize);
    }
    if (samplerCount > 0)
    {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = samplerCount * frameCount;
        poolSizes.push_back(poolSize);
    }

    VkDescriptorPoolCreateInfo poolCreateInfo{};
    poolCreateInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolCreateInfo.pPoolSizes    = poolSizes.data();
    poolCreateInfo.maxSets       = static_cast<uint32_t>(frameCount);

    VkResult result = vkCreateDescriptorPool(mDevice->getDevice(), &poolCreateInfo, nullptr, &mPool);
    if (result != VK_SUCCESS)
        throw std::runtime_error("failed to create descriptor pool");
}

void DescriptorPool::destroyPool()
{
    if (mPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(mDevice->getDevice(), mPool, nullptr);
        mPool = VK_NULL_HANDLE;
    }
}

} // namespace VKFW::vulkancore