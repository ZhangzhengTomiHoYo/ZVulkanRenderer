#pragma once
#include "Common.h"
#include "Platform/Window.h"
#include "VulkanCore/CommandPool.h"
#include "VulkanCore/Device.h"
#include "VulkanCore/Instance.h"
#include "VulkanCore/WindowSurface.h"
#include "VulkanCore/Swapchain.h"
#include "VulkanCore/RenderPass.h"
#include "VulkanCore/Pipeline.h"
#include "VulkanCore/CommandBuffer.h"
#include "VulkanCore/Semaphore.h"
#include "VulkanCore/Fence.h"
#include "VulkanCore/DataBuffer.h"

#include "Factories/RenderPassFactory.h"

namespace VKFW
{
class VulkanApp
{
  public:
    VulkanApp(int width = 1920, int height = 1080, bool framebufferResized = false) : mWidth(width), mHeight(height), mFramebufferResized(framebufferResized) {}
    void run();

  private:
    std::shared_ptr<platform::Window>          mWindow       = nullptr;
    std::shared_ptr<vulkancore::Instance>      mInstance     = nullptr;
    std::shared_ptr<vulkancore::WindowSurface> mSurface      = nullptr;
    std::shared_ptr<vulkancore::Device>        mDevice       = nullptr;
    std::shared_ptr<vulkancore::CommandPool>   mCommandPool  = nullptr;
    std::shared_ptr<vulkancore::SwapChain>     mSwapchain    = nullptr;
    std::shared_ptr<vulkancore::RenderPass>    mRenderPass   = nullptr;
    std::shared_ptr<vulkancore::Pipeline>      mPipeline     = nullptr;
    std::shared_ptr<vulkancore::DataBuffer>    mVertexBuffer = nullptr;

    std::vector<std::shared_ptr<vulkancore::CommandBuffer>> mCommandBuffers;

    std::vector<std::shared_ptr<vulkancore::Semaphore>> mImageAvailableSemaphores;
    std::vector<std::shared_ptr<vulkancore::Semaphore>> mRenderFinishedSemaphores;
    std::vector<std::shared_ptr<vulkancore::Fence>>     mFences;
    uint32_t                                            mCurrentFrame = 0;

    int  mWidth;
    int  mHeight;
    bool mFramebufferResized;

    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanUp();
    void onResize(int w, int h);

  private:
    void createSyncObjectes();
    std::shared_ptr<vulkancore::Pipeline> createPipeline();
    void recordCommandBuffers();
    void drawFrame();
};
} // namespace VKFW