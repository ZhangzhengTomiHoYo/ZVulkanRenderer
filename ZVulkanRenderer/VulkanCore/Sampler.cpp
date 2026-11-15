#include "Sampler.h"
#include <stdexcept>

namespace VKFW::vulkancore
{

Sampler::Sampler(const std::shared_ptr<Device> &device, bool isRepeat) : mDevice(device)
{
    auto addressMode = isRepeat ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, addressMode);
}

Sampler::~Sampler()
{
    if (mSampler != VK_NULL_HANDLE)
        vkDestroySampler(mDevice->getDevice(), mSampler, nullptr);
}

void Sampler::createSampler(VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode)
{
    VkSamplerCreateInfo samplerCreateInfo{};
    samplerCreateInfo.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.magFilter    = magFilter;
    samplerCreateInfo.minFilter    = minFilter;
    samplerCreateInfo.addressModeU = addressMode;
    samplerCreateInfo.addressModeV = addressMode;
    samplerCreateInfo.addressModeW = addressMode;

    samplerCreateInfo.anisotropyEnable = VK_TRUE;
    samplerCreateInfo.maxAnisotropy    = 16.0f;

    samplerCreateInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
    samplerCreateInfo.compareEnable           = VK_FALSE;
    samplerCreateInfo.compareOp               = VK_COMPARE_OP_ALWAYS;

    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCreateInfo.mipLodBias = 0.0f;
    samplerCreateInfo.minLod     = 0.0f;
    samplerCreateInfo.maxLod     = 0.0f;
    // TODO：为立方体贴图配置非无缝采样标志

    if (vkCreateSampler(mDevice->getDevice(), &samplerCreateInfo, nullptr, &mSampler) != VK_SUCCESS)
        throw std::runtime_error("failed to create sampler");
}

} // namespace VKFW::vulkancore