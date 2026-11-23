#include "Sampler.h"
#include <stdexcept>

namespace VKFW::vulkancore
{

Sampler::Sampler(const std::shared_ptr<Device> &device, bool isCubeMap, bool isRepeat) : mDevice(device)
{
    auto mode = isRepeat ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, mode, isCubeMap);
}

Sampler::~Sampler()
{
    if (mSampler != VK_NULL_HANDLE)
        vkDestroySampler(mDevice->getDevice(), mSampler, nullptr);
}

void Sampler::createSampler(VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode, bool isCubeMap)
{
    VkSamplerCreateInfo samplerCreateInfo{};
    samplerCreateInfo.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.magFilter    = magFilter;
    samplerCreateInfo.minFilter    = minFilter;
    samplerCreateInfo.addressModeU = isCubeMap ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : addressMode;
    samplerCreateInfo.addressModeV = isCubeMap ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : addressMode;
    samplerCreateInfo.addressModeW = isCubeMap ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : addressMode;

    samplerCreateInfo.anisotropyEnable = VK_TRUE;
    samplerCreateInfo.maxAnisotropy    = 16.0f;

    samplerCreateInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    // 非无缝 CubeMap 采样，与参考采样器一致；需要
    // VK_EXT_non_seamless_cube_map 扩展及 nonSeamlessCubeMap
    // 特性，两者均在创建设备时启用。
    samplerCreateInfo.flags                   = isCubeMap ? VK_SAMPLER_CREATE_NON_SEAMLESS_CUBE_MAP_BIT_EXT : 0;
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
    samplerCreateInfo.compareEnable           = VK_FALSE;
    samplerCreateInfo.compareOp               = VK_COMPARE_OP_ALWAYS;

    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCreateInfo.mipLodBias = 0.0f;
    samplerCreateInfo.minLod     = 0.0f;
    samplerCreateInfo.maxLod     = 4.0f;
    // TODO：为立方体贴图配置非无缝采样标志

    if (vkCreateSampler(mDevice->getDevice(), &samplerCreateInfo, nullptr, &mSampler) != VK_SUCCESS)
        throw std::runtime_error("failed to create sampler");
}

} // namespace VKFW::vulkancore