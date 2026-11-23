#pragma once
#include "Device.h"
#include <memory>

namespace VKFW::vulkancore
{

class Sampler
{
  public:
    // isCubeMap 为真时，启用非无缝 CubeMap 采样标志，
    // 并强制使用边缘钳制寻址，与参考采样器一致。
    // isRepeat 控制非 CubeMap 纹理使用重复还是边缘钳制寻址。
    Sampler(const std::shared_ptr<Device> &device, bool isCubeMap = false, bool isRepeat = false);
    ~Sampler();

    VkSampler getSampler() const { return mSampler; }
    // TODO：支持立方体贴图采样器

  private:
    void createSampler(VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode, bool isCubeMap);

    VkSampler               mSampler = VK_NULL_HANDLE;
    std::shared_ptr<Device> mDevice  = nullptr;
};

} // namespace VKFW::vulkancore