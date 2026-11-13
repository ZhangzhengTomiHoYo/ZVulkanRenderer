#pragma once
#include "../Common.h"

namespace VKFW::vulkancore
{
class Instance
{
  public:
    explicit Instance(bool enableValidationLayer = true);
    ~Instance();

    VkInstance getInstance() const { return mInstance; }

  private:
    VkInstance mInstance              = VK_NULL_HANDLE;
    bool       mEnableValidationLayer = true;

    VkDebugUtilsMessengerEXT mDebugger = VK_NULL_HANDLE;

    void setupDebugger();
    bool checkValidationLayerSupport();
    std::vector<const char *> getRequiredExtensions();
    void printAvailableExtensions();
    
    const std::vector<const char *> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
};
} 
