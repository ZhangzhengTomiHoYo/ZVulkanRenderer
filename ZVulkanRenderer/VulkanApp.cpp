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
}

void VulkanApp::mainLoop()
{
    while (!mWindow->shouldClose())
    {
        mWindow->pollEvents();
    }
}

void VulkanApp::cleanUp()
{
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
} // namespace VKFW
