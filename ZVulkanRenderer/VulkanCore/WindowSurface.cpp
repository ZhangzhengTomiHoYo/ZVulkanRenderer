#include "WindowSurface.h"
#include <stdexcept>

namespace VKFW::vulkancore
{

WindowSurface::WindowSurface(std::shared_ptr<Instance> instance, std::shared_ptr<platform::Window> window) : mInstance(instance)
{
    VkResult result = glfwCreateWindowSurface(instance->getInstance(), window->getWindow(), nullptr, &mSurface);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface");
    }
}

WindowSurface::~WindowSurface()
{
    if (mSurface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(mInstance->getInstance(), mSurface, nullptr);
    }
}

} // namespace VKFW::vulkancore
