#pragma once
#include "Common.h"
#include "Platform/Window.h"

namespace VKFW {
	class VulkanApp {
	public:
        VulkanApp(int width = 1920, int height = 1080, bool framebufferResized = false) : mWidth(width), mHeight(height), mFramebufferResized(framebufferResized) {}
		void run();

	private:
        std::shared_ptr<platform::Window> mWindow;

        int  mWidth;
        int  mHeight;
		bool mFramebufferResized;

		void initWindow();
		void initVulkan();
		void mainLoop();
		void cleanUp();
		void onResize(int w, int h);
	};
}