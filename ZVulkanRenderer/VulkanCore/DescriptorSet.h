#pragma once
#include "DescriptorPool.h"
#include "DescriptorSetLayout.h"
#include "Device.h"
#include "UniformParameter.h"
#include <memory>
#include <vector>

namespace VKFW::vulkancore
{

class DescriptorSet
{
  public:
    DescriptorSet(const std::shared_ptr<Device> &device, const std::vector<std::shared_ptr<UniformParameter>> &parameters, const std::shared_ptr<DescriptorSetLayout> &layout, const std::shared_ptr<DescriptorPool> &pool, int frameCount);
    ~DescriptorSet() = default;

    VkDescriptorSet getDescriptorSet(int frameIndex) const { return mDescriptorSets[frameIndex]; }

  private:
    std::vector<VkDescriptorSet> mDescriptorSets;
    std::shared_ptr<Device>      mDevice = nullptr;
};

} // namespace VKFW::vulkancore