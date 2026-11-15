#include "DescriptorSetLayout.h"
#include <stdexcept>

namespace VKFW::vulkancore
{

DescriptorSetLayout::DescriptorSetLayout(const std::shared_ptr<Device> &device) : mDevice(device) {}

DescriptorSetLayout::~DescriptorSetLayout()
{
    if (mLayout != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(mDevice->getDevice(), mLayout, nullptr);
}

void DescriptorSetLayout::build(const std::vector<std::shared_ptr<DescriptorParameter>> &parameters)
{
    mBindingParameters = parameters;

    if (mLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(mDevice->getDevice(), mLayout, nullptr);
        mLayout = VK_NULL_HANDLE;
    }

    std::vector<VkDescriptorSetLayoutBinding> bindings;
    for (const auto &parameter : parameters)
    {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding            = parameter->mBinding;
        binding.descriptorType     = parameter->mDescriptorType;
        binding.descriptorCount    = parameter->mCount;
        binding.stageFlags         = parameter->mStageFlags;
        binding.pImmutableSamplers = nullptr;
        bindings.push_back(binding);
    }

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutCreateInfo.pBindings    = bindings.data();

    VkResult result = vkCreateDescriptorSetLayout(mDevice->getDevice(), &layoutCreateInfo, nullptr, &mLayout);
    if (result != VK_SUCCESS)
        throw std::runtime_error("failed to create descriptor set layout");
}

} // namespace VKFW::vulkancore