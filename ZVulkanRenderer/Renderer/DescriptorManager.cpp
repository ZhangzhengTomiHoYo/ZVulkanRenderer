#include "DescriptorManager.h"

namespace VKFW::renderer
{

void DescriptorManager::init(const std::shared_ptr<vulkancore::Device> &device, int frameCount)
{
    mDevice     = device;
    mFrameCount = frameCount;
}

void DescriptorManager::attachCameraUBO()
{
    auto parameter             = std::make_shared<vulkancore::DescriptorParameter>();
    parameter->mBinding        = static_cast<uint32_t>(mParams.size());
    parameter->mDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    parameter->mCount          = 1;
    parameter->mStageFlags     = VK_SHADER_STAGE_VERTEX_BIT;
    parameter->mSize           = sizeof(CameraUBO);

    for (int i = 0; i < mFrameCount; i++)
        parameter->mBuffers.push_back(vulkancore::DataBuffer::createUniformBuffer(mDevice, parameter->mSize));

    mParams.push_back(parameter);
}

void DescriptorManager::attachObjectUBO()
{
    auto parameter             = std::make_shared<vulkancore::DescriptorParameter>();
    parameter->mBinding        = static_cast<uint32_t>(mParams.size());
    parameter->mDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    parameter->mCount          = 1;
    parameter->mStageFlags     = VK_SHADER_STAGE_VERTEX_BIT;
    parameter->mSize           = sizeof(ObjectUBO);

    for (int i = 0; i < mFrameCount; i++)
        parameter->mBuffers.push_back(vulkancore::DataBuffer::createUniformBuffer(mDevice, parameter->mSize));

    mParams.push_back(parameter);
}
void DescriptorManager::attachTexture(const std::shared_ptr<vulkancore::Texture> &texture)
{
    auto parameter = std::make_shared<vulkancore::DescriptorParameter>();

    parameter->mBinding        = static_cast<uint32_t>(mParams.size());
    parameter->mDescriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    parameter->mSize           = 1;
    parameter->mStageFlags     = VK_SHADER_STAGE_FRAGMENT_BIT;

    for (int i = 0; i < mFrameCount; i++)
        parameter->mTextures.push_back(texture);

    mParams.push_back(parameter);
}

void DescriptorManager::build()
{
    mLayout = std::make_shared<vulkancore::DescriptorSetLayout>(mDevice);
    mLayout->build(mParams);

    mPool = std::make_shared<vulkancore::DescriptorPool>(mDevice);
    mPool->build(mParams, mFrameCount);

    mDescriptorSet = std::make_shared<vulkancore::DescriptorSet>(mDevice, mParams, mLayout, mPool, mFrameCount);
}

void DescriptorManager::updateCameraUBO(const CameraUBO &data, int frameIndex)
{
    mParams[0]->mBuffers[frameIndex]->updateBufferByMap(&data, sizeof(CameraUBO));
}

void DescriptorManager::updateObjectUBO(const ObjectUBO &data, int frameIndex)
{
    mParams[1]->mBuffers[frameIndex]->updateBufferByMap(&data, sizeof(ObjectUBO));
}

} // namespace VKFW::renderer