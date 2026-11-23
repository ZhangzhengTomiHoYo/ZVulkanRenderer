#include "Device.h"
#include <set>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>

namespace VKFW::vulkancore
{

Device::Device(std::shared_ptr<Instance> instance, std::shared_ptr<WindowSurface> surface) : mInstance(instance), mSurface(surface)
{
    choosePhysicalDevice();
    initQueueFamilies(mPhysicalDevice);
    createLogicalDevice();
}

Device::~Device()
{
    vkDestroyDevice(mDevice, nullptr);
}

VkSampleCountFlagBits Device::getMaxUsableSampleCount() const
{
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(mPhysicalDevice, &props);

    // 只有颜色和深度都支持的采样数才可用
    VkSampleCountFlags counts = props.limits.framebufferColorSampleCounts & props.limits.framebufferDepthSampleCounts;

    if (counts & VK_SAMPLE_COUNT_64_BIT)
        return VK_SAMPLE_COUNT_64_BIT;
    if (counts & VK_SAMPLE_COUNT_32_BIT)
        return VK_SAMPLE_COUNT_32_BIT;
    if (counts & VK_SAMPLE_COUNT_16_BIT)
        return VK_SAMPLE_COUNT_16_BIT;
    if (counts & VK_SAMPLE_COUNT_8_BIT)
        return VK_SAMPLE_COUNT_8_BIT;
    if (counts & VK_SAMPLE_COUNT_4_BIT)
        return VK_SAMPLE_COUNT_4_BIT;
    if (counts & VK_SAMPLE_COUNT_2_BIT)
        return VK_SAMPLE_COUNT_2_BIT;
    return VK_SAMPLE_COUNT_1_BIT;
}

void Device::choosePhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(mInstance->getInstance(), &deviceCount, nullptr);
    if (deviceCount == 0)
    {
        throw std::runtime_error("no Vulkan-capable GPU found");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(mInstance->getInstance(), &deviceCount, devices.data());

    for (const auto &device : devices)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(device, &features);

        // 必须支持交换链扩展
        uint32_t extCount = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, extensions.data());

        bool hasSwapchain = false;
        for (const auto &ext : extensions)
        {
            if (std::string(ext.extensionName) == VK_KHR_SWAPCHAIN_EXTENSION_NAME)
            {
                hasSwapchain = true;
                break;
            }
        }
        if (!hasSwapchain)
            continue;

        // 必须支持图像和呈现队列族
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        bool hasGraphics = false, hasPresent = false;

        for (uint32_t i = 0; i < queueFamilyCount; i++)
        {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                hasGraphics      = true;
                VkBool32 present = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, mSurface->getSurface(), &present);
                if (present)
                {
                    hasPresent = true;
                }
            }
        }
        if (hasGraphics && hasPresent)
        {
            mPhysicalDevice = device;
            spdlog::info("GPU: {}", properties.deviceName);
            return;
        }
    }

    throw std::runtime_error("no suitable GPU found");
}

void Device::initQueueFamilies(VkPhysicalDevice device)
{
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

    for (uint32_t i = 0; i < count; i++)
    {
        if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            // 设置图形队列索引
            mGraphicQueueFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, mSurface->getSurface(), &presentSupport);
        if (presentSupport)
        {
            // 设置呈现队列索引
            mPresentQueueFamily = i;
        }
    }

    if (!isQueueFamilyComplete())
    {
        throw std::runtime_error("queue families incomplete");
    }
}

bool Device::isQueueFamilyComplete()
{
    return mGraphicQueueFamily.has_value() && mPresentQueueFamily.has_value();
}

// 创建逻辑设备时 设置图形与呈现队列
void Device::createLogicalDevice()
{
    // 图形队列族和呈现队列族通常是一个 使用集合去重
    std::set<uint32_t> uniqueFamilies = {mGraphicQueueFamily.value(), mPresentQueueFamily.value()};

    float                                priority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    for (uint32_t family : uniqueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = family;
        queueCreateInfo.queueCount       = 1;
        queueCreateInfo.pQueuePriorities = &priority;
        queueCreateInfos.push_back(queueCreateInfo);
    }


    VkPhysicalDeviceFeatures features{};
    features.samplerAnisotropy = VK_TRUE;

    VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT e{};
    e.sType              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_NON_SEAMLESS_CUBE_MAP_FEATURES_EXT;
    e.nonSeamlessCubeMap = VK_TRUE;

    VkPhysicalDeviceVulkan11Features features11{};
    features11.sType                = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    features11.shaderDrawParameters = VK_TRUE;
    features11.pNext                = &e;



    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext                   = &features11;
    deviceCreateInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos       = queueCreateInfos.data();
    deviceCreateInfo.pEnabledFeatures        = &features;
    deviceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    VkResult result = vkCreateDevice(mPhysicalDevice, &deviceCreateInfo, nullptr, &mDevice);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create logical device");
    }

    vkGetDeviceQueue(mDevice, mGraphicQueueFamily.value(), 0, &mGraphicQueue);
    vkGetDeviceQueue(mDevice, mPresentQueueFamily.value(), 0, &mPresentQueue);
}

} // namespace VKFW::vulkancore
