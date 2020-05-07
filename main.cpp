#define GLFW_INCLUDE_VULKAN 
#include "GLFW/glfw3.h"
#include "VulkanEnv.h"
#include "RenderingData.h"
#include "ModelImport.h"
#include <iostream>

const char* APP_TITLE = "vulkan";
const uint32_t WIDTH = 400;
const uint32_t HEIGHT = 400;

struct RenderContext {
	VulkanEnv* vulkanEnv;
	RenderingData* renderingData;
};

static void onFramebufferResize(GLFWwindow *window, int width, int height) {
	auto* renderContext = reinterpret_cast<RenderContext*>(glfwGetWindowUserPointer(window));
	renderContext->vulkanEnv->onFramebufferResize();
	if (height > 0) {
		renderContext->renderingData->setAspectRatio(width / (float)height);
	}
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

	VertexIndexed tetrahedron = {
		{
			{{0.0f, -0.577f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
			{{0.0f, 0.289f, 0.577f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
			{{-0.5f, 0.289f, -0.289f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.0f}},
			{{0.5f, 0.289f, -0.289f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}
		},
		{
			0, 1, 2,
			0, 2, 3,
			0, 3, 1,
			3, 2, 1,
		}
	};
	MeshInput inputTetrahedron(tetrahedron, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f });
	VertexIndexed cube = {
		{
			{{-0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
			{{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
			{{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.0f}},
			{{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
			{{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
			{{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
			{{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.0f}},
			{{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		},
		{
			4, 5, 6, 6, 7, 4,//back
			0, 1, 2, 2, 3, 0,//front
			4, 0, 3, 3, 5, 4,//left
			1, 7, 6, 6, 2, 1,//right
			5, 3, 2, 2, 6, 5,//bottom
			0, 4, 7, 7, 1, 0,//top
		}
	};
	MeshInput inputCube(cube, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f });
	RenderingData renderingData;
	renderingData.updateCamera(60.0f, WIDTH / (float)HEIGHT, glm::vec3(0.0f, -1.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	VulkanEnv vulkanEnv;
	vulkanEnv.setWindow(window);
	vulkanEnv.setRenderingData(renderingData);
	vulkanEnv.setMaxFrameInFlight(2);
	vulkanEnv.setUniformSize(renderingData.getUniformSize());
	vulkanEnv.setMsaaSample(8);
	RenderContext renderContext = { &vulkanEnv, &renderingData };

	ImageInput imageInput(false, true);
	imageInput.setMipLevel(3);
	logResult(imageInput.load("texture/vibrant-watercolor-flow-texture-background_1017-19544.jpg"));

	logResult(vulkanEnv.createInstance(APP_TITLE));
	logResult(vulkanEnv.createSurface());
	logResult(vulkanEnv.createPhysicalDevice());
	logResult(vulkanEnv.createDevice());
	logResult(vulkanEnv.createSwapchain());
	logResult(vulkanEnv.createSwapchainImageView());
	logResult(vulkanEnv.createMsaaColorBuffer());
	logResult(vulkanEnv.createDepthBuffer());
	logResult(vulkanEnv.createRenderPass());
	logResult(vulkanEnv.createDescriptorSetLayout());
	logResult(vulkanEnv.createGraphicsPipelineLayout());
	logResult(vulkanEnv.createGraphicsPipeline());
	logResult(vulkanEnv.createFrameBuffer());
	logResult(vulkanEnv.setupFence());
	logResult(vulkanEnv.createCommandPool());
	logResult(vulkanEnv.createTextureImage(imageInput));
	logResult(vulkanEnv.createTextureImageView());
	logResult(vulkanEnv.createTextureSampler());
	logResult(vulkanEnv.createVertexBufferIndice({ &inputTetrahedron, &inputCube }));
	logResult(vulkanEnv.createUniformBuffer());
	logResult(vulkanEnv.createDescriptorPool());
	logResult(vulkanEnv.createDescriptorSet());
	logResult(vulkanEnv.allocateSwapchainCommandBuffer());
	logResult(vulkanEnv.createFrameSyncObject());
	logResult(vulkanEnv.updateUniformBuffer());

	std::cout << std::endl;

	glfwSetWindowUserPointer(window, &renderContext);
	glfwSetFramebufferSizeCallback(window, onFramebufferResize);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		inputTetrahedron.animate(90);
		inputCube.animate(45);
		vulkanEnv.drawFrame(renderingData);
	}

	vulkanEnv.waitUntilIdle();
	vulkanEnv.destroy();

	glfwDestroyWindow(window);
	glfwTerminate();
}