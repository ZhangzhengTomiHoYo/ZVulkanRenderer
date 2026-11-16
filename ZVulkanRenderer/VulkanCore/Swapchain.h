#pragma once
#include "../Platform/Window.h"
#include "Device.h"
#include "WindowSurface.h"
#include "CommandPool.h"
#include "Image.h"
#include <limits>
#include <memory>
#include <vector>
namespace VKFW::vulkancore
{
class RenderPass; // forward declaration
}

namespace VKFW::vulkancore
{

struct SwapChainSupportInfo
{
    VkSurfaceCapabilitiesKHR        mCapabilities = {};
    std::vector<VkSurfaceFormatKHR> mFormats;
    std::vector<VkPresentModeKHR>   mPresentModes;
};

class SwapChain
{
  public:
    SwapChain(const std::shared_ptr<Device> &device, const std::shared_ptr<platform::Window> &window, const std::shared_ptr<WindowSurface> &surface, const std::shared_ptr<CommandPool>& commandPool);
    ~SwapChain();

    void createFrameBuffers(const std::shared_ptr<RenderPass> &renderPass);

    void createResources();
    void recreateSwapchain();
    void releaseResources();

    SwapChainSupportInfo querySwapChainSupportInfo();
    VkSurfaceFormatKHR   chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats);
    VkPresentModeKHR     choosePresentMode(const std::vector<VkPresentModeKHR> &modes);
    VkExtent2D           chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

    VkFormat getSwapChainImageFormat() const { return mSwapChainFormat; }
    VkExtent2D getSwapChainExtent() const { return mSwapChainExtent; }
    VkSwapchainKHR getSwapChain() const { return mSwapChain; }
    uint32_t getImageCount() const { return mImageCount; }
    std::vector<VkImage> getSwapChainImages() const { return mSwapChainImages; }
    std::vector<VkImageView> getSwapChainImageViews() const { return mSwapChainImageViews; }
    std::vector<VkFramebuffer> getSwapChainFramebuffers() const { return mSwapChainFramebuffers; }

  private:
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels = 1);

    std::shared_ptr<Device>           mDevice  = nullptr;
    std::shared_ptr<platform::Window> mWindow  = nullptr;
    std::shared_ptr<WindowSurface>    mSurface = nullptr;
    std::shared_ptr<CommandPool>      mCommandPool = nullptr;

    VkSwapchainKHR mSwapChain       = VK_NULL_HANDLE;
    VkFormat       mSwapChainFormat = {};
    VkExtent2D     mSwapChainExtent = {};
    uint32_t       mImageCount      = 0;                     // ½»»»Á´Í¼ÏñÊýÁ¿

    std::vector<VkImage>       mSwapChainImages       = {};
    std::vector<std::shared_ptr<Image>> mDepthImages           = {};
    std::vector<VkImageView>   mSwapChainImageViews   = {};
    std::vector<VkFramebuffer> mSwapChainFramebuffers = {};
};

} // namespace VKFW::vulkancore
