#pragma once
#include "../Common.h"
#include "Instance.h"
#include "WindowSurface.h"
#include <memory>
#include <optional>
#include <vector>

namespace VKFW::vulkancore
{

class Device
{
  public:
    Device(std::shared_ptr<Instance> instance, std::shared_ptr<WindowSurface> surface);
    ~Device();

    VkPhysicalDevice getPhysicalDevice() const { return mPhysicalDevice; }

    VkDevice getDevice() const { return mDevice; }
    VkQueue getGraphicQueue() const { return mGraphicQueue; }
    VkQueue getPresentQueue() const { return mPresentQueue; }
    std::optional<uint32_t> getGraphicQueueFamily() const { return mGraphicQueueFamily; }
    std::optional<uint32_t> getPresentQueueFamily() const { return mPresentQueueFamily; }

  private:
    void choosePhysicalDevice();

    void initQueueFamilies(VkPhysicalDevice device);
    bool isQueueFamilyComplete();
    void createLogicalDevice();

    VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
    VkDevice         mDevice         = VK_NULL_HANDLE;

    std::shared_ptr<Instance>      mInstance = nullptr;
    std::shared_ptr<WindowSurface> mSurface  = nullptr;

    std::optional<uint32_t> mGraphicQueueFamily;
    std::optional<uint32_t> mPresentQueueFamily;
    VkQueue                 mGraphicQueue = VK_NULL_HANDLE;
    VkQueue                 mPresentQueue = VK_NULL_HANDLE;

    const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
};

} // namespace VKFW::vulkancore
