#include "Common.h"
#include "VulkanApp.h"
#include <spdlog/spdlog.h>

int main()
{
	spdlog::info("Starting ZVulkanRenderer");

	VKFW::VulkanApp myApp;

	try {
		myApp.run();
	}
	catch (const std::runtime_error& e) {
		spdlog::error("{}", e.what());
	}

	return 0;
}
