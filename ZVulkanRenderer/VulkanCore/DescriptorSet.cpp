#include "DescriptorSet.h"
#include <stdexcept>

namespace VKFW::vulkancore
{

DescriptorSet::DescriptorSet(const std::shared_ptr<Device> &device, const std::vector<std::shared_ptr<DescriptorParameter>> &parameters, const std::shared_ptr<DescriptorSetLayout> &layout, const std::shared_ptr<DescriptorPool> &pool, int frameCount) : mDevice(device)
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

    // 为每一帧写入缓冲区或纹理描述符
    for (int i = 0; i < frameCount; i++)
    {
        std::vector<VkWriteDescriptorSet> writes;

        for (const auto &parameter : parameters)
        {
            VkWriteDescriptorSet write{};
            write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet          = mDescriptorSets[i];
            write.dstBinding      = parameter->mBinding;
            write.dstArrayElement = 0;
            write.descriptorType  = parameter->mDescriptorType;
            write.descriptorCount = parameter->mCount;

            if (parameter->mDescriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            {
                write.pBufferInfo = &parameter->mBuffers[i]->getBufferInfo();
            }
            else if (parameter->mDescriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
            {
                write.pImageInfo = &parameter->mTextures[i]->getImageInfo();
            }
            else
            {
                continue; // 跳过尚未支持的描述符类型
            }

            writes.push_back(write);
        }

        vkUpdateDescriptorSets(mDevice->getDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}

} // namespace VKFW::vulkancore
