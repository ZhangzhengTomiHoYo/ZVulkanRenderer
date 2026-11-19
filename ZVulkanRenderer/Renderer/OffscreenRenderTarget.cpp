#include "OffscreenRenderTarget.h"
#include <array>
#include <stdexcept>

namespace VKFW::renderer
{
OffscreenRenderTarget::OffscreenRenderTarget(const std::shared_ptr<vulkancore::Device> &device, const std::shared_ptr<vulkancore::CommandPool> &commandPool, uint32_t width, uint32_t height, uint32_t imageCount, VkFormat colorFormat, VkFormat depthFormat, VkImageLayout renderTargetFinalLayout) : mDevice(device), mCommandPool(commandPool), mWidth(width), mHeight(height), mImageCount(imageCount), mColorFormat(colorFormat), mDepthFormat(depthFormat)
{
    createImageEntities();
    createRenderPass(renderTargetFinalLayout);
    createFramebuffer();
}

OffscreenRenderTarget::~OffscreenRenderTarget()
{
    cleanup();
}

void OffscreenRenderTarget::cleanup()
{
    for (auto &framebuffer : mOffScreenFramebuffers)
    {
        vkDestroyFramebuffer(mDevice->getDevice(), framebuffer, nullptr); // 销毁 Framebuffer
    }
    mRenderPass.reset();
    mDepthAttachment.reset();
    mClearValues.clear();
}

void OffscreenRenderTarget::createImageEntities()
{
    mRenderTargetImages.resize(mImageCount);
    mMultisampleImages.resize(mImageCount);
    mDepthImages.resize(mImageCount);

    for (uint32_t i = 0; i < mImageCount; ++i)
    {
        mRenderTargetImages[i] = vulkancore::Image::createRenderTargetImage(mDevice, mWidth, mHeight, mColorFormat);
        mRenderTargetImages[i]->transitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, mCommandPool);

        mMultisampleImages[i] = vulkancore::Image::createMultiSampleImage(mDevice, mWidth, mHeight, mColorFormat); // 创建 MSAA 颜色图像
        mMultisampleImages[i]->transitionLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, mCommandPool); // 设置 MSAA 颜色图像布局

        mDepthImages[i] = vulkancore::Image::createDepthStencil(mDevice, mWidth, mHeight, mDepthFormat, mDevice->getMaxUsableSampleCount());
        mDepthImages[i]->transitionLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, mCommandPool); // 设置深度图像布局
    }
}

void OffscreenRenderTarget::createRenderPass(VkImageLayout renderTargetFinalLayout)
{
    mRenderPass = factories::RenderPassFactory::CreateRenderPassWithMSAA(mDevice, mColorFormat, mDevice->getMaxUsableSampleCount(), mDepthFormat, renderTargetFinalLayout);
}

void OffscreenRenderTarget::createFramebuffer()
{
    // 为每组离屏图像创建 Framebuffer。
    mOffScreenFramebuffers.resize(mImageCount); // 分配 Framebuffer 数组
    for (size_t i = 0; i < mImageCount; i++)
    {
        // Framebuffer 将本次绘制需要的附件图像视图组合起来。
        // 附件顺序必须与 RenderPass 中的附件描述一致。
        // 每组离屏图像分别创建一个 Framebuffer。
        // 注意附件顺序。
        std::array<VkImageView, 3> attachments{
            mMultisampleImages[i]->getImageView(),  // MSAA 颜色图像
            mRenderTargetImages[i]->getImageView(), // 解析结果，供下一遍绘制读取
            mDepthImages[i]->getImageView()         // 深度模板附件图像试图
        };       
        VkFramebufferCreateInfo    framebufferInfo{};
        framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass      = mRenderPass->getRenderPass();              // 对应的 RenderPass
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size()); // 附件数量
        framebufferInfo.pAttachments    = attachments.data();                        // 附件图像视图
        framebufferInfo.width           = mWidth;                                    // 宽度
        framebufferInfo.height          = mHeight;                                   // 高度
        framebufferInfo.layers          = 1;                                         // 图层数量
        if (vkCreateFramebuffer(mDevice->getDevice(), &framebufferInfo, nullptr, &mOffScreenFramebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Error: Failed to create offscreen framebuffer!");
        }
    }
}

void OffscreenRenderTarget::recreate(uint32_t width, uint32_t height)
{
    mWidth  = width;
    mHeight = height;

    // 先销毁 Framebuffer，因为它们引用旧图像视图。
    for (auto &framebuffer : mOffScreenFramebuffers)
        vkDestroyFramebuffer(mDevice->getDevice(), framebuffer, nullptr);
    mOffScreenFramebuffers.clear();

    // 释放旧图像。
    mRenderTargetImages.clear();
    mMultisampleImages.clear();
    mDepthImages.clear();

    // 格式和采样数未变，继续使用现有 RenderPass。
    createImageEntities();
    createFramebuffer();
}

} // namespace VKFW::renderer
