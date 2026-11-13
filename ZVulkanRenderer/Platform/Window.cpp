#include "Window.h"

namespace VKFW::platform {
	Window::Window(int width, int height) :mWidth(width), mHeight(height) {
		if (!glfwInit()) {
			throw std::runtime_error("glfwInit failed");
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		mWindow = glfwCreateWindow(mWidth, mHeight, "ZVulkanRenderer Window", nullptr, nullptr);

		glfwSetWindowUserPointer(mWindow, this);
		// 注册回调函数 framebuffer_size_callback(), glfwPollEvents()轮询时发现窗口尺寸变化就回调
		glfwSetFramebufferSizeCallback(mWindow, framebuffer_size_callback);
	}
	void Window::framebuffer_size_callback(GLFWwindow* window, int width, int height)
	{
		auto self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
        self->mWidth  = width;
        self->mHeight = height;
		// mOnResize() 也是一个回调 由VulkanApp自己实现 复杂改变交换链帧缓冲等资源上的图像大小
        self->mOnResize(width, height);
	}
	Window::~Window() {
		if (mWindow) glfwDestroyWindow(mWindow);
		glfwTerminate();
	}

	bool Window::shouldClose() const {
		return mWindow && glfwWindowShouldClose(mWindow);
	}
	void Window::pollEvents() {
		glfwPollEvents();
	}
}