#include "Window.h"

namespace VKFW::platform {
	Window::Window(int width, int height) :mWidth(width), mHeight(height) {
		if (!glfwInit()) {
			throw std::runtime_error("glfwInit failed");
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		mWindow = glfwCreateWindow(mWidth, mHeight, "ZVulkanRenderer Window", nullptr, nullptr);

        if (!mWindow)
        {
            glfwTerminate();
            throw std::runtime_error("glfwCreateWindow failed");
        }

		glfwSetWindowUserPointer(mWindow, this);
		// 注册回调函数 framebuffer_size_callback(), glfwPollEvents()轮询时发现窗口尺寸变化就回调
		glfwSetFramebufferSizeCallback(mWindow, framebuffer_size_callback);
        glfwSetCursorPosCallback(mWindow, cursor_pos_callback);
	}
    void Window::cursor_pos_callback(GLFWwindow *window, double xpos, double ypos) 
    {
        auto self = static_cast<Window *>(glfwGetWindowUserPointer(window));

        if (self->mOnMouseMove)
            self->mOnMouseMove(xpos, ypos);
    }

    void Window::framebuffer_size_callback(GLFWwindow *window, int width, int height)
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
    void Window::processEvents() 
	{
        if (!mWindow)
            return;

        if (glfwGetKey(mWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(mWindow, true);

        if (!mOnKeyMove)
            return;

        if (glfwGetKey(mWindow, GLFW_KEY_W) == GLFW_PRESS)
            mOnKeyMove(CameraMove::Front);

        if (glfwGetKey(mWindow, GLFW_KEY_S) == GLFW_PRESS)
            mOnKeyMove(CameraMove::Back);

        if (glfwGetKey(mWindow, GLFW_KEY_A) == GLFW_PRESS)
            mOnKeyMove(CameraMove::Left);

        if (glfwGetKey(mWindow, GLFW_KEY_D) == GLFW_PRESS)
            mOnKeyMove(CameraMove::Right);
    }
    bool Window::isLeftMousePressed() const
    {
        return mWindow && glfwGetMouseButton(mWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    }
    }