#include "DescriptorSet.h"
#include <stdexcept>

namespace VKFW::vulkancore
{

DescriptorSet::DescriptorSet(const std::shared_ptr<Device> &device, const std::vector<std::shared_ptr<UniformParameter>> &parameters, const std::shared_ptr<DescriptorSetLayout> &layout, const std::shared_ptr<DescriptorPool> &pool, int frameCount) : mDevice(device)
{
    mDescriptorSets.resize(frameCount);
    std::vector<VkDescriptorSetLayout> layouts(frameCount, layout->getLayout());

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool     = pool->getDescriptorPool();
    allocateInfo.descriptorSetCount = static_cast<uint32_t>(frameCount);
    allocateInfo.pSetLayouts        = layouts.data();

    VkResult result = vkAllocateDescriptorSets(mDevice->getDevice(), &allocateInfo, mDescriptorSets.data());
    if (result != VK_SUCCESS)
        throw std::runtime_error("failed to allocate descriptor sets");

    // 为每一帧写入 UBO 描述符
    for (int i = 0; i < frameCount; i++)
    {
        std::vector<VkWriteDescriptorSet> writes;

        for (const auto &parameter : parameters)
        {
            if (parameter->mDescriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                continue; // TODO：处理组合图像采样器描述符

            VkWriteDescriptorSet write{};
            write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet          = mDescriptorSets[i];
            write.dstBinding      = parameter->mBinding;
            write.dstArrayElement = 0;
            write.descriptorType  = parameter->mDescriptorType;
            write.descriptorCount = parameter->mCount;
            write.pBufferInfo     = &parameter->mBuffers[i]->getBufferInfo();
            writes.push_back(write);
        }

        vkUpdateDescriptorSets(mDevice->getDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}

} // namespace VKFW::vulkancore