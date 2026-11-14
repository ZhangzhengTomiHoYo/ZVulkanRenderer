#include "VulkanApp.h"
#include <spdlog/spdlog.h>

namespace VKFW
{
void VulkanApp::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanUp();
}

void VulkanApp::initWindow()
{
    mWindow = std::make_shared<platform::Window>(mWidth, mHeight);
    mWindow->setOnResize([this](int w, int h) { this->onResize(w, h); });
}

void VulkanApp::initVulkan()
{
    mInstance    = std::make_shared<vulkancore::Instance>(true);
    mSurface     = std::make_shared<vulkancore::WindowSurface>(mInstance, mWindow);
    mDevice      = std::make_shared<vulkancore::Device>(mInstance, mSurface);
    mCommandPool = std::make_shared<vulkancore::CommandPool>(mDevice);
    mSwapchain   = std::make_shared<vulkancore::SwapChain>(mDevice, mWindow, mSurface);

    spdlog::info("Instance + Surface + Device + CommandPool + Swapchain ready");

    mRenderPass = createRenderPass();

    // === Framebuffer 创建 ===
    // Swapchain RenderPass Framebuffer 三者密不可分 (离屏渲染除外)
    // Framebuffer的创建依赖RenderPass 并绑定Swapchain的Imageview
    // 本项目Framebuffer封装在Swapchain中
    mSwapchain->createFrameBuffers(mRenderPass);
}

void VulkanApp::mainLoop()
{
    while (!mWindow->shouldClose())
    {
        mWindow->pollEvents();
        drawFrame();
    }
}

void VulkanApp::cleanUp()
{
    mRenderPass.reset();
    mSwapchain.reset();
    mCommandPool.reset();
    mDevice.reset();
    mSurface.reset();
    mInstance.reset();
    mWindow.reset();
}

void VulkanApp::onResize(int w, int h)
{
    mWidth              = w;
    mHeight             = h;
    mFramebufferResized = true;
    spdlog::info("resized: {}x{}", w, h);
}

std::shared_ptr<vulkancore::RenderPass> VulkanApp::createRenderPass()
{
    /*
        RenderPass
        ├─ 附件描述[0]：颜色图像应该怎样使用
        ├─ Subpass[0]
        │  └─ 颜色输出[0]：使用上面的附件[0]
        └─ 依赖[0]：规定外部操作与 Subpass 0 之间的同步
    */

    auto renderPass = std::make_shared<vulkancore::RenderPass>(mDevice);

    VkAttachmentDescription colorAttachDescrip{};
    colorAttachDescrip.format         = mSwapchain->getSwapChainImageFormat();
    colorAttachDescrip.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorAttachDescrip.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachDescrip.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachDescrip.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachDescrip.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachDescrip.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;        // RenderPass 可以同一张附件指定 开始 和 结束 时一共两种状态
    colorAttachDescrip.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    renderPass->addAttachmentDescription(colorAttachDescrip);

    VkAttachmentReference colorAttachRef{};
    colorAttachRef.attachment = 0;
    colorAttachRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // SubPass 只能指定这一时刻下 这一张附件 的 一个状态

    // SubPass 加入 颜色附件引用
    vulkancore::SubPass subpass;
    subpass.addColorAttachmentReference(colorAttachRef);

    renderPass->addSubpass(subpass);

    /* ↑
        RenderPass 开始          Subpass 0 写颜色             RenderPass 结束
        UNDEFINED          →    COLOR_ATTACHMENT_OPTIMAL  → PRESENT_SRC_KHR
        initialLayout            layout                       finalLayout
    */

    VkSubpassDependency dep1{};
    dep1.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dep1.dstSubpass    = 0;
    dep1.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep1.srcAccessMask = 0;
    dep1.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep1.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    renderPass->addDependency(dep1);

    renderPass->buildRenderPass();

    return renderPass;
}

void VulkanApp::drawFrame()
{
    // 由Vulkan驱动决定下一张被渲染的交换链图像
    uint32_t imageIndex = 0;
    vkAcquireNextImageKHR(mDevice->getDevice(), mSwapchain->getSwapChain(), UINT64_MAX, VK_NULL_HANDLE, VK_NULL_HANDLE, &imageIndex);

    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool        = mCommandPool->getCommandPool();
    allocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(mDevice->getDevice(), &allocateInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    //vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkClearValue clearColor{};
    clearColor.color = {1.0f, 0.0f, 0.4f, 1.0f};

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass        = mRenderPass->getRenderPass();
    renderPassBeginInfo.framebuffer       = mSwapchain->getSwapChainFramebuffers()[imageIndex];
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = mSwapchain->getSwapChainExtent();
    renderPassBeginInfo.clearValueCount   = 1;
    renderPassBeginInfo.pClearValues      = &clearColor;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        // TODO drawcall
        vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);

    // 命令录制完成  下面开始提交
    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer;

    vkQueueSubmit(mDevice->getGraphicQueue(), 1, &submitInfo, VK_NULL_HANDLE);

    vkQueueWaitIdle(mDevice->getGraphicQueue());

    vkFreeCommandBuffers(mDevice->getDevice(), mCommandPool->getCommandPool(), 1, &commandBuffer);

    // 需要一个左值
    VkSwapchainKHR swapchain = mSwapchain->getSwapChain();

    // 绘制完成  下面开始呈现已经绘制好的交换链图像
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType          = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains    = &swapchain;
    presentInfo.pImageIndices  = &imageIndex;

    vkQueuePresentKHR(mDevice->getPresentQueue(), &presentInfo);
}

} // namespace VKFW
