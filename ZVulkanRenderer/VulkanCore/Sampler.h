#pragma once
#include "Device.h"
#include <memory>

namespace VKFW::vulkancore
{

class Sampler
{
  public:
    Sampler(const std::shared_ptr<Device> &device, bool isRepeat = false);
    ~Sampler();

    VkSampler getSampler() const { return mSampler; }
    // TODO：支持立方体贴图采样器

  private:
    void createSampler(VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode);

    VkSampler               mSampler = VK_NULL_HANDLE;
    std::shared_ptr<Device> mDevice  = nullptr;
};

} // namespace VKFW::vulkancore