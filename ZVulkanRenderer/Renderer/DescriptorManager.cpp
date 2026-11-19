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
    mCameraParamIndex = static_cast<int>(mParams.size());

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
    mObjectParamIndex = static_cast<int>(mParams.size());

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
    parameter->mCount          = 1;
    parameter->mStageFlags     = VK_SHADER_STAGE_FRAGMENT_BIT;

    for (int i = 0; i < mFrameCount; i++)
        parameter->mTextures.push_back(texture);

    mParams.push_back(parameter);
}

void DescriptorManager::attachPerFrameTextures(const std::vector<std::shared_ptr<vulkancore::Texture>> &perFrameTextures) 
{
    if (static_cast<int>(perFrameTextures.size()) != mFrameCount)
        throw std::runtime_error("attachPerFrameTextures: texture count must equal frame count");

    auto parameter = std::make_shared<vulkancore::DescriptorParameter>();

    parameter->mBinding        = static_cast<uint32_t>(mParams.size());
    parameter->mDescriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    parameter->mCount          = 1;
    parameter->mStageFlags     = VK_SHADER_STAGE_FRAGMENT_BIT;

    for (int i = 0; i < mFrameCount; i++)
        parameter->mTextures.push_back(perFrameTextures[i]);

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
    if (mCameraParamIndex < 0)
        throw std::runtime_error("DescriptorManager: no camera UBO attached");
    mParams[mCameraParamIndex]->mBuffers[frameIndex]->updateBufferByMap(&data, sizeof(CameraUBO));
}

void DescriptorManager::updateObjectUBO(const ObjectUBO &data, int frameIndex)
{
    if (mObjectParamIndex < 0)
        throw std::runtime_error("DescriptorManager: no object UBO attached");
    mParams[mObjectParamIndex]->mBuffers[frameIndex]->updateBufferByMap(&data, sizeof(ObjectUBO));
}

} // namespace VKFW::renderer