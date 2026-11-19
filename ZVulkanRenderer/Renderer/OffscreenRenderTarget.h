#pragma once

#include "../Factories/RenderPassFactory.h"
#include "../VulkanCore/CommandBuffer.h"
#include "../VulkanCore/CommandPool.h"
#include "../VulkanCore/Device.h"
#include "../VulkanCore/Image.h"
#include "../VulkanCore/RenderPass.h"
#include "Renderer_Common.h"
#include <memory>
#include <vector>

namespace VKFW::renderer
{
class OffscreenRenderTarget
{
  public:
    OffscreenRenderTarget(const std::shared_ptr<vulkancore::Device> &device, const std::shared_ptr<vulkancore::CommandPool> &commandPool, uint32_t width, uint32_t height, uint32_t imageCount, VkFormat colorFormat, VkFormat depthFormat, VkImageLayout renderTargetFinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    ~OffscreenRenderTarget();

  public:
    void recreate(uint32_t width, uint32_t height); // 窗口尺寸变化后重建

    std::shared_ptr<vulkancore::Image>                     getRenderTargetImage(int index) const { return mRenderTargetImages[index]; }
    const std::vector<std::shared_ptr<vulkancore::Image>> &getRenderTargetImages() const { return mRenderTargetImages; }

    std::vector<VkFramebuffer>              getOffScreenFramebuffers() const { return mOffScreenFramebuffers; }
    std::shared_ptr<vulkancore::RenderPass> getRenderPass() const { return mRenderPass; }

    uint32_t getWidth() const { return mWidth; }
    uint32_t getHeight() const { return mHeight; }

  private:
    void createImageEntities();
    void createRenderPass(VkImageLayout renderTargetFinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    void createFramebuffer();
    void cleanup();

  private:
    std::shared_ptr<vulkancore::Device>      mDevice;
    std::shared_ptr<vulkancore::CommandPool> mCommandPool;
    uint32_t                                 mWidth, mHeight;
    uint32_t                                 mImageCount; // 离屏目标数量，默认与交换链图像数量一致
    int                                      mColorBufferCount;
    VkFormat                                 mColorFormat, mDepthFormat;

    // 单采样解析结果，供下一遍绘制读取。
    std::vector<std::shared_ptr<vulkancore::Image>> mRenderTargetImages{}; // 离屏渲染目标

    // MSAA 深度图像，用于深度测试。
    std::vector<std::shared_ptr<vulkancore::Image>> mDepthImages{};

    // MSAA 颜色图像，用于场景绘制。
    std::vector<std::shared_ptr<vulkancore::Image>> mMultisampleImages{};

    std::shared_ptr<vulkancore::Image> mDepthAttachment;
    std::vector<VkClearValue>          mClearValues;
    int                                mDepthBufferIndex;

    std::shared_ptr<vulkancore::RenderPass> mRenderPass = nullptr;
    std::vector<VkFramebuffer>              mOffScreenFramebuffers{};
};
} // namespace VKFW::renderer