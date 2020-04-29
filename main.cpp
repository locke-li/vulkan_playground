#define GLFW_INCLUDE_VULKAN 
#include "GLFW/glfw3.h"
#include "VulkanEnv.h"
#include "RenderingData.h"
#include <iostream>

const char* APP_TITLE = "vulkan";
const uint32_t WIDTH = 400;
const uint32_t HEIGHT = 400;

static void onFramebufferResize(GLFWwindow *window, int width, int height) {
	auto* vulkanEnv = reinterpret_cast<VulkanEnv*>(glfwGetWindowUserPointer(window));
	vulkanEnv->onFramebufferResize();
}

static void logResult(bool result) {
	std::cout << result << "\n";
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
	VertexInput vertexInput;
	RenderingData renderingData;

	ImageInput imageInput;
	logResult(imageInput.load("texture/vibrant-watercolor-flow-texture-background_1017-19544.jpg"));

	logResult(vulkanEnv.createInstance(APP_TITLE));
	logResult(vulkanEnv.createSurface());
	logResult(vulkanEnv.createPhysicalDevice());
	logResult(vulkanEnv.createDevice());
	logResult(vulkanEnv.createSwapchain());
	logResult(vulkanEnv.createSwapchainImageView());
	logResult(vulkanEnv.createRenderPass());
	logResult(vulkanEnv.createDescriptorSetLayout());
	logResult(vulkanEnv.createGraphicsPipelineLayout());
	logResult(vulkanEnv.createGraphicsPipeline());
	logResult(vulkanEnv.createFrameBuffer());
	logResult(vulkanEnv.setupFence());
	logResult(vulkanEnv.createCommandPool());
	logResult(vulkanEnv.createTextureImage(imageInput, false));
	logResult(vulkanEnv.createTextureImageView());
	logResult(vulkanEnv.createTextureSampler());
	logResult(vulkanEnv.createVertexBufferIndice({ &vertexInput }));
	logResult(vulkanEnv.createUniformBuffer());
	logResult(vulkanEnv.createDescriptorPool());
	logResult(vulkanEnv.createDescriptorSet());
	logResult(vulkanEnv.allocateSwapchainCommandBuffer());
	logResult(vulkanEnv.setupCommandBuffer());
	logResult(vulkanEnv.createFrameSyncObject());

	std::cout << std::endl;

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