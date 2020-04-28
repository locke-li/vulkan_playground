#define GLFW_INCLUDE_VULKAN 
#include "GLFW/glfw3.h"
#include "VulkanEnv.h"
#include <iostream>

const char* APP_TITLE = "vulkan";
const uint32_t WIDTH = 400;
const uint32_t HEIGHT = 200;

static void onFramebufferResize(GLFWwindow *window, int width, int height) {
	auto* vulkanEnv = reinterpret_cast<VulkanEnv*>(glfwGetWindowUserPointer(window));
	vulkanEnv->onFramebufferResize();
}

int main(int argc, char** argv) {
	if (!glfwInit()) {
		return 1;
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	auto* window = glfwCreateWindow(WIDTH, HEIGHT, APP_TITLE, nullptr, nullptr);
	VulkanEnv vulkanEnv;
	vulkanEnv.setWindow(window);
	vulkanEnv.setMaxFrameInFlight(2);
	VertexInput fixedVertexInput;

	if (!vulkanEnv.createInstance(APP_TITLE)) {
		return 2;
	}
	if (!vulkanEnv.createSurface()) {
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
	if (!vulkanEnv.createRenderPass()) {
		return 8;
	}
	if (!vulkanEnv.createGraphicsPipelineLayout()) {
		return 9;
	}
	if (!vulkanEnv.createGraphicsPipeline()) {
		return 10;
	}
	if (!vulkanEnv.createFrameBuffer()) {
		return 11;
	}
	if (!vulkanEnv.createCommandPool()) {
		return 12;
	}
	if (!vulkanEnv.setupBufferCopy()) {
		return 13;
	}
	if (!vulkanEnv.createVertexBufferIndice({&fixedVertexInput})) {
		return 14;
	}
	if (!vulkanEnv.allocateCommandBuffer()) {
		return 15;
	}
	if (!vulkanEnv.setupCommandBuffer()) {
		return 16;
	}
	if (!vulkanEnv.createFrameSyncObject()) {
		return 17;
	}

	glfwSetWindowUserPointer(window, &vulkanEnv);
	glfwSetFramebufferSizeCallback(window, onFramebufferResize);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		vulkanEnv.drawFrame();
	}

	vulkanEnv.waitUntilIdle();
	vulkanEnv.destroy();

	glfwDestroyWindow(window);
	glfwTerminate();
}