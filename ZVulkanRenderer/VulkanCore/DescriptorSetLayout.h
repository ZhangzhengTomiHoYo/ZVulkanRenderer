#pragma once
#include "Device.h"
#include "DescriptorParameter.h"
#include <memory>
#include <vector>

namespace VKFW::vulkancore
{

class DescriptorSetLayout
{
  public:
    DescriptorSetLayout(const std::shared_ptr<Device> &device);
    ~DescriptorSetLayout();

    void build(const std::vector<std::shared_ptr<DescriptorParameter>> &parameters);

    VkDescriptorSetLayout getLayout() const { return mLayout; }
    std::shared_ptr<Device> getDevice() const { return mDevice; }
    std::vector<std::shared_ptr<DescriptorParameter>> getBindingParameters() const { return mBindingParameters; }

  private:
    VkDescriptorSetLayout                          mLayout = VK_NULL_HANDLE;
    std::shared_ptr<Device>                        mDevice = nullptr;
    std::vector<std::shared_ptr<DescriptorParameter>> mBindingParameters;
};

} // namespace VKFW::vulkancore