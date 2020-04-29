#define GLFW_INCLUDE_VULKAN 
#include "GLFW/glfw3.h"
#include "VulkanEnv.h"
#include "RenderingData.h"
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
	RenderingData renderingData;
	ImageInput imageInput;
	imageInput.load("texture/vibrant-watercolor-flow-texture-background_1017-19544.jpg");

	vulkanEnv.createInstance(APP_TITLE);
	vulkanEnv.createSurface();
	vulkanEnv.createPhysicalDevice();
	vulkanEnv.createDevice();
	vulkanEnv.createSwapchain();
	vulkanEnv.createSwapchainImageView();
	vulkanEnv.createRenderPass();
	vulkanEnv.createDescriptorSetLayout();
	vulkanEnv.createGraphicsPipelineLayout();
	vulkanEnv.createGraphicsPipeline();
	vulkanEnv.createFrameBuffer();
	vulkanEnv.setupFence();
	vulkanEnv.createCommandPool();
	vulkanEnv.createTextureImage(imageInput, false);
	vulkanEnv.createTextureImageView();
	vulkanEnv.createTextureSampler();
	vulkanEnv.createVertexBufferIndice({ &fixedVertexInput });
	vulkanEnv.createUniformBuffer();
	vulkanEnv.createDescriptorPool();
	vulkanEnv.createDescriptorSet();
	vulkanEnv.allocateSwapchainCommandBuffer();
	vulkanEnv.setupCommandBuffer();
	vulkanEnv.createFrameSyncObject();

	glfwSetWindowUserPointer(window, &vulkanEnv);
	glfwSetFramebufferSizeCallback(window, onFramebufferResize);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		renderingData.updateUniform(vulkanEnv.getWidth(), vulkanEnv.getHeight());
		vulkanEnv.drawFrame(renderingData);
	}

	vulkanEnv.waitUntilIdle();
	vulkanEnv.destroy();

	glfwDestroyWindow(window);
	glfwTerminate();
}