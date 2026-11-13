#pragma once
#include "Common.h"
#include "Platform/Window.h"
#include "VulkanCore/CommandPool.h"
#include "VulkanCore/Device.h"
#include "VulkanCore/Instance.h"
#include "VulkanCore/WindowSurface.h"
#include "VulkanCore/Swapchain.h"

namespace VKFW
{
class VulkanApp
{
  public:
    VulkanApp(int width = 1920, int height = 1080, bool framebufferResized = false) : mWidth(width), mHeight(height), mFramebufferResized(framebufferResized) {}
    void run();

  private:
    std::shared_ptr<platform::Window>          mWindow      = nullptr;
    std::shared_ptr<vulkancore::Instance>      mInstance    = nullptr;
    std::shared_ptr<vulkancore::WindowSurface> mSurface     = nullptr;
    std::shared_ptr<vulkancore::Device>        mDevice      = nullptr;
    std::shared_ptr<vulkancore::CommandPool>   mCommandPool = nullptr;
    std::shared_ptr<vulkancore::SwapChain>     mSwapchain   = nullptr;

    int  mWidth;
    int  mHeight;
    bool mFramebufferResized;

    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanUp();
    void onResize(int w, int h);
};
} // namespace VKFW