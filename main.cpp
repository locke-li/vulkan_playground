#define GLFW_INCLUDE_VULKAN 
#include "GLFW/glfw3.h"
#include "VulkanEnv.h"
#include <iostream>

const char* APP_TITLE = "vulkan";
const uint32_t WIDTH = 400;
const uint32_t HEIGHT = 200;

int main(int argc, char** argv) {
	if (!glfwInit()) {
		return 1;
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	auto* window = glfwCreateWindow(WIDTH, HEIGHT, APP_TITLE, nullptr, nullptr);
	VulkanEnv vulkanEnv;
	vulkanEnv.setExtent(WIDTH, HEIGHT);

	if (!vulkanEnv.createInstance(APP_TITLE)) {
		return 2;
	}
	if (!vulkanEnv.createSurface(window)) {
		return 3;
	}
	if (!vulkanEnv.createPhysicalDevice()) {
		return 4;
	}
	if (!vulkanEnv.createDevice()) {
		return 5;
	}
	if (!vulkanEnv.createSwapchain()) {
		return 6;
	}
	if (!vulkanEnv.createImageView()) {
		return 7;
	}
	if (!vulkanEnv.createPipeline()) {
		
	}

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}

	vulkanEnv.destroy();

	glfwDestroyWindow(window);
	glfwTerminate();
}