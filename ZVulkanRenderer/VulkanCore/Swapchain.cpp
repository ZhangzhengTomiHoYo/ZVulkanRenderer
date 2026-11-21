#include "Swapchain.h"
#include "RenderPass.h"
#include <algorithm>
#include <array>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace VKFW::vulkancore
{

SwapChain::SwapChain(const std::shared_ptr<Device> &device, const std::shared_ptr<platform::Window> &window, const std::shared_ptr<WindowSurface> &surface, const std::shared_ptr<CommandPool> &commandPool) 
    : mDevice(device), mWindow(window), mSurface(surface), mCommandPool(commandPool)
{
    createResources();
}

SwapChain::~SwapChain()
{
    releaseResources();
}

void SwapChain::createFrameBuffers(const std::shared_ptr<RenderPass> &renderPass)
{
    mSwapChainFramebuffers.resize(mImageCount);
    for (size_t i = 0; i < mImageCount; i++)
    {
        std::array<VkImageView, 2> attachments = {
            mSwapChainImageViews[i], 
            mDepthImages[i]->getImageView()
        };

        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass      = renderPass->getRenderPass();
        framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferCreateInfo.pAttachments    = attachments.data();
        framebufferCreateInfo.width           = mSwapChainExtent.width;
        framebufferCreateInfo.height          = mSwapChainExtent.height;
        framebufferCreateInfo.layers          = 1;
        VkResult result = vkCreateFramebuffer(mDevice->getDevice(), &framebufferCreateInfo, nullptr, &mSwapChainFramebuffers[i]);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create framebuffer");
        }
    }
    spdlog::info("Framebuffers created: {}", mImageCount);
}

void SwapChain::createResources() 
{
    // === 查询支持情况，并选择交换链配置 ===
    auto info        = querySwapChainSupportInfo();           // 查询支持情况
    auto format      = chooseSurfaceFormat(info.mFormats);    // 图像格式和色彩空间
    auto presentMode = choosePresentMode(info.mPresentModes); // 呈现模式
    auto extent      = chooseSwapExtent(info.mCapabilities);  // 尺寸

    spdlog::info("chosen: format={}  presentMode={}  extent={}x{}", static_cast<int>(format.format), static_cast<int>(presentMode), extent.width, extent.height);

    uint32_t minImageCount = info.mCapabilities.minImageCount + 1;
    if (info.mCapabilities.maxImageCount > 0 && minImageCount > info.mCapabilities.maxImageCount)
        minImageCount = info.mCapabilities.maxImageCount;

    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface          = mSurface->getSurface();
    swapchainCreateInfo.minImageCount    = minImageCount;
    swapchainCreateInfo.imageColorSpace  = format.colorSpace;
    swapchainCreateInfo.imageFormat      = format.format;
    swapchainCreateInfo.imageExtent      = extent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    std::vector<uint32_t> indices = {mDevice->getGraphicQueueFamily().value(), mDevice->getPresentQueueFamily().value()};

    if (indices[0] == indices[1])
    {
        swapchainCreateInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0;
        swapchainCreateInfo.pQueueFamilyIndices   = nullptr;
    }
    else
    {
        swapchainCreateInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(indices.size());
        swapchainCreateInfo.pQueueFamilyIndices   = indices.data();
    }

    swapchainCreateInfo.preTransform   = info.mCapabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode    = presentMode;
    swapchainCreateInfo.clipped        = VK_TRUE;

    // vkCreateSwapchainKHR 内部会创建Image 但还没给图像句柄
    VkResult result = vkCreateSwapchainKHR(mDevice->getDevice(), &swapchainCreateInfo, nullptr, &mSwapChain);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("vkCreateSwapchainKHR failed.");
    }

    mSwapChainFormat = format.format;
    mSwapChainExtent = extent;

    vkGetSwapchainImagesKHR(mDevice->getDevice(), mSwapChain, &mImageCount, nullptr);
    mSwapChainImages.resize(mImageCount);
    vkGetSwapchainImagesKHR(mDevice->getDevice(), mSwapChain, &mImageCount, mSwapChainImages.data()); // 获取已经存在的图像句柄

    spdlog::info("Swapchain created: {}x{} actual images = {}", mSwapChainExtent.width, mSwapChainExtent.height, mImageCount);

    // ↑所以是: 创建交换链 → 查询实际图像数量 → 调整数组大小 → 获取已有图像的句柄

    // === 创建图像视图 ===
    mSwapChainImageViews.resize(mImageCount);
    for (size_t i = 0; i < mImageCount; i++)
    {
        mSwapChainImageViews[i] = createImageView(mSwapChainImages[i], mSwapChainFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }

    mDepthImages.resize(mImageCount);
    for (size_t i = 0; i < mImageCount; i++)
    {
        mDepthImages[i] = Image::createDepthStencil(mDevice, mSwapChainExtent.width, mSwapChainExtent.height, VK_FORMAT_D24_UNORM_S8_UINT);
        mDepthImages[i]->transitionLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, mCommandPool);
    }
}

void SwapChain::recreateSwapchain() 
{
    VkResult result = vkDeviceWaitIdle(mDevice->getDevice());
    if (result != VK_SUCCESS)
        throw std::runtime_error("vkDeviceWaitIdle before swapchain recreation failed: VkResult=" + std::to_string(result));

    int w, h;
    glfwGetFramebufferSize(mWindow->getWindow(), &w, &h);
    while (w == 0 || h == 0)
    {
        glfwWaitEvents();
        glfwGetFramebufferSize(mWindow->getWindow(), &w, &h);
    }

    releaseResources();
    createResources();
}

void SwapChain::releaseResources() 
{
    for (auto framebuffer : mSwapChainFramebuffers)
        vkDestroyFramebuffer(mDevice->getDevice(), framebuffer, nullptr);

    for (auto imageView : mSwapChainImageViews)
        vkDestroyImageView(mDevice->getDevice(), imageView, nullptr);

    if (mSwapChain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(mDevice->getDevice(), mSwapChain, nullptr);
}

VkImageView SwapChain::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
    VkImageViewCreateInfo imageViewCreateInfo{};
    imageViewCreateInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.image    = image;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format   = format;

    imageViewCreateInfo.subresourceRange.aspectMask     = aspectFlags;
    imageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
    imageViewCreateInfo.subresourceRange.levelCount     = mipLevels;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount     = 1;

    VkImageView imageView = VK_NULL_HANDLE;
    VkResult    result    = vkCreateImageView(mDevice->getDevice(), &imageViewCreateInfo, nullptr, &imageView);
    if (result != VK_SUCCESS)
        throw std::runtime_error("failed to create image view");

    return imageView;
}

// 查询物理设备上交换链支持情况
SwapChainSupportInfo SwapChain::querySwapChainSupportInfo()
{
    SwapChainSupportInfo info;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mDevice->getPhysicalDevice(), mSurface->getSurface(), &info.mCapabilities);

    uint32_t count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(mDevice->getPhysicalDevice(), mSurface->getSurface(), &count, nullptr);
    if (count)
    {
        info.mFormats.resize(count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(mDevice->getPhysicalDevice(), mSurface->getSurface(), &count, info.mFormats.data());
    }

    vkGetPhysicalDeviceSurfacePresentModesKHR(mDevice->getPhysicalDevice(), mSurface->getSurface(), &count, nullptr);
    if (count)
    {
        info.mPresentModes.resize(count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(mDevice->getPhysicalDevice(), mSurface->getSurface(), &count, info.mPresentModes.data());
    }
    return info;
}

VkSurfaceFormatKHR SwapChain::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats)
{
    for (const auto &surfaceFormat : formats)
        if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM && surfaceFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
            return surfaceFormat;
    return formats[0];
}

VkPresentModeKHR SwapChain::choosePresentMode(const std::vector<VkPresentModeKHR> &modes)
{
    for (const auto &presentMode : modes)
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return presentMode;
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;

    int width = 0, height = 0;
    glfwGetFramebufferSize(mWindow->getWindow(), &width, &height);

    VkExtent2D extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    extent.width      = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    extent.height     = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    return extent;
}

} // namespace VKFW::vulkancore
