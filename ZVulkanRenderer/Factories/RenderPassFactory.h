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
    static std::shared_ptr<vulkancore::RenderPass> CreateRenderPassWithDepth(
        const std::shared_ptr<vulkancore::Device>& device,
        VkFormat colorFormat,
        VkFormat depthFormat = VK_FORMAT_D24_UNORM_S8_UINT
    );
    static std::shared_ptr<vulkancore::RenderPass> CreateRenderPassWithMSAA(
        const std::shared_ptr<vulkancore::Device> &device,
		VkFormat colorFormat,
		VkSampleCountFlagBits msaaSamples,
		VkFormat depthFormat = VK_FORMAT_D24_UNORM_S8_UINT,
		VkImageLayout imageFinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
};
} // namespace VKFW::factories