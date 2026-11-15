#pragma once
#include "Device.h"
#include "DescriptorParameter.h"
#include <memory>
#include <vector>

namespace VKFW::vulkancore
{

class DescriptorPool
{
  public:
    DescriptorPool(const std::shared_ptr<Device> &device);
    ~DescriptorPool();

    void build(const std::vector<std::shared_ptr<DescriptorParameter>> &parameters, int frameCount);

    VkDescriptorPool getDescriptorPool() const { return mPool; }

  private:
    void destroyPool();

    VkDescriptorPool        mPool   = VK_NULL_HANDLE;
    std::shared_ptr<Device> mDevice = nullptr;
    // TODO：支持组合图像采样器描述符
};

} // namespace VKFW::vulkancore