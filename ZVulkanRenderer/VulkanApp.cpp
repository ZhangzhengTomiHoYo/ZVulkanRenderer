#include "VulkanApp.h"
#include <spdlog/spdlog.h>

namespace VKFW
{
void VulkanApp::run()
{
    initWindow();
    initVulkan();
    initScene();
    createSyncObjectes();
    mainLoop();
    cleanUp();
}

void VulkanApp::initWindow()
{
    mWindow = std::make_shared<platform::Window>(mWidth, mHeight);
    mWindow->setOnResize([this](int w, int h) { this->onResize(w, h); });
    mWindow->setOnKeyMove([this](platform::CameraMove move) { onKeyMove(move); });
    mWindow->setOnMouseMove([this](double xpos, double ypos) { onMouseMove(xpos, ypos); });
}

void VulkanApp::initVulkan()
{
    mInstance    = std::make_shared<vulkancore::Instance>(true);
    mSurface     = std::make_shared<vulkancore::WindowSurface>(mInstance, mWindow);
    mDevice      = std::make_shared<vulkancore::Device>(mInstance, mSurface);
    mCommandPool = std::make_shared<vulkancore::CommandPool>(mDevice);
    mSwapchain   = std::make_shared<vulkancore::SwapChain>(mDevice, mWindow, mSurface, mCommandPool);
    mRenderPass  = factories::RenderPassFactory::CreateRenderPassWithDepth(mDevice, mSwapchain->getSwapChainImageFormat());

    // === Framebuffer 创建 ===
    // Swapchain RenderPass Framebuffer 三者密不可分 (离屏渲染除外)
    // Framebuffer的创建依赖RenderPass 并绑定Swapchain的Imageview
    // 本项目Framebuffer封装在Swapchain中
    mSwapchain->createFrameBuffers(mRenderPass);

    spdlog::info("Instance + Surface + Device + CommandPool + Swapchain + RenderPass + Framebuffer ready");

    auto extent = mSwapchain->getSwapChainExtent();
    mOffscreenRT = std::make_shared<renderer::OffscreenRenderTarget>(
        mDevice, mCommandPool, 
        extent.width, extent.height, 
        mSwapchain->getImageCount(), VK_FORMAT_R32G32B32A32_SFLOAT, 
        vulkancore::Image::findDepthFormat(mDevice)
    );
}

void VulkanApp::initScene() 
{
    mScene = std::make_shared<renderer::Scene>(mDevice, mCommandPool, mSwapchain, mRenderPass, mOffscreenRT);
    mScene->init();
}

void VulkanApp::mainLoop()
{
    double fpsTimer = 0.0f;
    int    frameCount = 0;
    while (!mWindow->shouldClose())
    {
        mWindow->pollEvents();
        mWindow->processEvents();
        
        float dt = getFrameTime();
        
        fpsTimer += dt;
        frameCount++;

        if (fpsTimer >= 1.0)
        {
            std::string title = "ZVulkanRenderer | FPS: " + std::to_string(static_cast<int>(frameCount / fpsTimer));

            glfwSetWindowTitle(mWindow->getWindow(), title.c_str());

            fpsTimer   = 0.0;
            frameCount = 0;
        }

        mScene->update(dt, mCurrentFrame);

        drawFrame();
    }
    vkDeviceWaitIdle(mDevice->getDevice());
}

void VulkanApp::cleanUp()
{
    //mCommandBuffers.clear();
    //mPipeline.reset();
    vkDeviceWaitIdle(mDevice->getDevice());
    mScene.reset();
    mOffscreenRT.reset();
    mRenderPass.reset();
    mSwapchain.reset();
    mCommandPool.reset();
    mDevice.reset();
    mSurface.reset();
    mInstance.reset();
    mWindow.reset();
}

// 用于回调
void VulkanApp::onResize(int w, int h)
{
    mWidth              = w;
    mHeight             = h;
    mFramebufferResized = true;
    spdlog::info("resized: {}x{}", w, h);
}

renderer::CameraMove VulkanApp::toCameraMove(platform::CameraMove move)
{
    switch (move)
    {
    case platform::CameraMove::Front:
        return renderer::CameraMove::Front;
    case platform::CameraMove::Back:
        return renderer::CameraMove::Back;
    case platform::CameraMove::Left:
        return renderer::CameraMove::Left;
    case platform::CameraMove::Right:
        return renderer::CameraMove::Right;
    }

    return renderer::CameraMove::Front;
}

void VulkanApp::recreateSwapchain() 
{
    mSwapchain->recreateSwapchain();
    mSwapchain->createFrameBuffers(mRenderPass);

    auto extent = mSwapchain->getSwapChainExtent();
    mOffscreenRT->recreate(extent.width, extent.height);

    mScene->onResize();

    createSyncObjectes();
    mCurrentFrame = 0;
}

void VulkanApp::createSyncObjectes()
{
    const uint32_t count = mSwapchain->getImageCount();
    mImageAvailableSemaphores.clear();
    mRenderFinishedSemaphores.clear();
    mFences.clear();

    for (uint32_t i = 0; i < count; i++)
    {
        mImageAvailableSemaphores.push_back(std::make_shared<vulkancore::Semaphore>(mDevice));
        mRenderFinishedSemaphores.push_back(std::make_shared<vulkancore::Semaphore>(mDevice));
        mFences.push_back(std::make_shared<vulkancore::Fence>(mDevice, true)); // Fence初始化为Signaled
    }
}

void VulkanApp::drawFrame()
{
    // 1. 等待GPU 将 Fence 置为 Signaled 
    mFences[mCurrentFrame]->waitForFence();

    // 2. 由Vulkan驱动决定下一张被渲染的交换链图像
    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(
        mDevice->getDevice(), 
        mSwapchain->getSwapChain(), 
        UINT64_MAX, 
        mImageAvailableSemaphores[mCurrentFrame]->getSemaphore(),   // 此时 imageIndex 还没有获取  同步资源只能使用mCurrentFrame获取
        VK_NULL_HANDLE, 
        &imageIndex
    );
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || mFramebufferResized)
    {
        recreateSwapchain();
        mFramebufferResized = false;
        return;
    }
    if (result != VK_SUCCESS)
        throw std::runtime_error("vkAcquireNextImageKHR failed: VkResult=" + std::to_string(result));

    // 3. 手动重置Fence
    mFences[mCurrentFrame]->resetFence();

    // 获取已经录制好的命令
    VkCommandBuffer commandBuffer = mScene->getCommandBuffer(imageIndex)->getCommandBuffer();

    VkSemaphore          waitSemphores[]    = {mImageAvailableSemaphores[mCurrentFrame]->getSemaphore()};
    VkSemaphore          signalSemaphores[] = {mRenderFinishedSemaphores[mCurrentFrame]->getSemaphore()};
    VkPipelineStageFlags waitStages[]       = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};             // 只有在向颜色附件写入时才等待

    // 4. 提交
    VkSubmitInfo submitInfo{};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = waitSemphores;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphores;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &commandBuffer;
    submitInfo.pWaitDstStageMask    = waitStages;

    result = vkQueueSubmit(mDevice->getGraphicQueue(), 1, &submitInfo, mFences[mCurrentFrame]->getFence());
    if (result != VK_SUCCESS)
        throw std::runtime_error("vkQueueSubmit failed: VkResult=" + std::to_string(result));

    VkSwapchainKHR swapchains[] = {mSwapchain->getSwapChain()};

    // 5. 绘制完成(等signalSemaphores)  但是可以先提交呈现请求
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = signalSemaphores;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = swapchains;
    presentInfo.pImageIndices      = &imageIndex;

    result = vkQueuePresentKHR(mDevice->getPresentQueue(), &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || mFramebufferResized)
    {
        recreateSwapchain();
        mFramebufferResized = false;
        return;
    }

    if (result != VK_SUCCESS)
        throw std::runtime_error("vkQueuePresentKHR failed: VkResult=" + std::to_string(result));

    // 6. 下一帧使用的同步资源下标
    mCurrentFrame = (mCurrentFrame + 1) % mSwapchain->getImageCount();
}

float VulkanApp::getFrameTime()
{
    static double lastTime = 0.0f;
    double        currentTime = glfwGetTime();
    float         deltaTime   = static_cast<float>(currentTime - lastTime);
    lastTime                  = currentTime;
    return deltaTime;
}

void VulkanApp::onKeyMove(platform::CameraMove move) 
{
    if (!mScene)
        return;

    auto camera = mScene->getMainCamera();
    if (camera)
        camera->move(toCameraMove(move));
}

void VulkanApp::onMouseMove(double xpos, double ypos) 
{
    if (!mScene)
        return;

    auto camera = mScene->getMainCamera();
    if (!camera)
        return;

    if (!mWindow->isLeftMousePressed())
    {
        camera->resetMouseState();
        return;
    }

    camera->onMouseMove(xpos, ypos);
}

} // namespace VKFW
