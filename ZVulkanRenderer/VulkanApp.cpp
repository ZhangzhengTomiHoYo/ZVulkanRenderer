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
        mWindow->setOnResize(
            [this](int w, int h) {
                this->onResize(w, h);
            });
    }

    void VulkanApp::initVulkan()
    {
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
        mWindow.reset();
    }
    void VulkanApp::onResize(int w, int h)
    {
        mWidth  = w;
        mHeight = h;
        mFramebufferResized = true;
        spdlog::info("resized: {}x{}", w, h);
    }
} // namespace VKFW
