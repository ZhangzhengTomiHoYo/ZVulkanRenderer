#pragma once
#include "../VulkanCore/Device.h"
#include "../VulkanCore/RenderPass.h"
#include <memory>

namespace VKFW::factories
{
class RenderPassFactory
{
  public:
    static std::shared_ptr<vulkancore::RenderPass> CreateSimpleRenderPass(const std::shared_ptr<vulkancore::Device> &device, VkFormat colorFormat);
};
} // namespace VKFW::factories