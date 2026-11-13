#pragma once
#include "../platform/Window.h"
#include "Instance.h"
#include <memory>

namespace VKFW::vulkancore
{

class WindowSurface
{
  public:
    WindowSurface(std::shared_ptr<Instance> instance, std::shared_ptr<platform::Window> window);
    ~WindowSurface();

    VkSurfaceKHR getSurface() const { return mSurface; }
    VkInstance getInstance() const { return mInstance->getInstance(); }

  private:
    VkSurfaceKHR              mSurface  = VK_NULL_HANDLE;
    std::shared_ptr<Instance> mInstance = nullptr;
};

} // namespace VKFW::vulkancore
