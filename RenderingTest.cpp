#include "RenderingTest.h"
#include "ModelImport.h"
#include "ShaderInput.h"
#include "DebugHelper.hpp"
#include "MeshNode.h"
#include <iostream>
#include <fstream>
#include <chrono>

const char* APP_TITLE = "vulkan";
const uint32_t WIDTH = 400;
const uint32_t HEIGHT = 400;

void onFramebufferResize(GLFWwindow* window, int width, int height) {
	auto* renderContext = reinterpret_cast<RenderingTest::RenderContext*>(glfwGetWindowUserPointer(window));
	renderContext->vulkanEnv->onFramebufferResize();
	if (height > 0) {
		renderContext->renderingData->setAspectRatio(width / (float)height);
	}
}

bool RenderingTest::readInput(TestInput& inputOut) const {
	std::ifstream input("_input");
	if (!input.is_open()) {
		return false;
	}
	//NOTE for efficiency, go with C style per char parsing
	std::string line;
	while(std::getline(input, line)) {
		if (line.length() == 0 || strncmp(line.c_str(), "#", 1) == 0) continue;
		auto delimIndex = line.find('=', 0);
		std::string key = line.substr(0, delimIndex);
		++delimIndex;
		if (key == "model") {
			inputOut.modelPath = line.substr(delimIndex);
		}
		else if (key == "texture") {
			inputOut.texturePath = line.substr(delimIndex);
		}
		else if (key == "vertex_shader") {
			inputOut.vertexShaderPath = line.substr(delimIndex);
		}
		else if (key == "fragment_shader") {
			inputOut.fragmentShaderPath = line.substr(delimIndex);
		}
		//silent ignore unrecognized
		std::cout << line << std::endl;
	}
	return true;
}

 void RenderingTest::prepareModel(const TestInput& input) {
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
	 MeshInput inputTetrahedron{ { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f, 0.0f }, {1.0f, 1.0f, 1.0f} };
	 inputTetrahedron.setMesh(std::move(tetrahedron));
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
	 MeshInput inputCube{ { 0.0f, 0.0f, -2.0f }, { 1.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } };
	 inputCube.setMesh(std::move(cube));
	 MeshInput inputLoadedModel{ { 0.2f, 0.0f, 0.0f }, glm::quat({0.0f, glm::radians(-45.0f), glm::radians(180.0f)}), {1.0f, 1.0f, 1.0f} };
	 ModelImport modelImport;
	 logResult("model loading", modelImport.load(input.modelPath, 28, &inputLoadedModel));
	 modelList.push_back(std::move(inputTetrahedron));
	 modelList.push_back(std::move(inputCube));
	 modelList.push_back(std::move(inputLoadedModel));
 }

int RenderingTest::mainLoop() {
	TestInput input;
	if (!readInput(input)) {
		return 2;
	}

	if (!windowLayer.init()) {
		return 1;
	}
	windowLayer.createWindow(APP_TITLE, WIDTH, HEIGHT);

	const auto& graphicsSetting = setting.getGraphics();
	prepareModel(input);
	renderingData.updateCamera(45.0f, WIDTH / (float)HEIGHT, glm::vec3(0.0f, -1.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	renderingData.setRenderListFiltered(modelList);
	ShaderInput shader(input.vertexShaderPath, input.fragmentShaderPath);
	shader.preload();
	vulkanEnv.setWindow(windowLayer.getWindow());
	vulkanEnv.setRenderingData(renderingData);
	vulkanEnv.setShader(shader);
	vulkanEnv.setMaxFrameInFlight(graphicsSetting.MaxFrameInFlight);
	vulkanEnv.setUniformSize(renderingData.getUniformSize());
	vulkanEnv.setMsaaSample(graphicsSetting.MSAASample);
	renderContext.vulkanEnv = &vulkanEnv;
	renderContext.renderingData = &renderingData;

	ImageInput imageInput(false, true);
	imageInput.setMipLevel(3);
	logResult("texture loading", imageInput.load(input.texturePath));

	logResult("create instance", vulkanEnv.createInstance(APP_TITLE));
	logResult("create surface", vulkanEnv.createSurface());
	logResult("create physical device", vulkanEnv.createPhysicalDevice());
	logResult("create logical device", vulkanEnv.createDevice());
	logResult("create swapchain", vulkanEnv.createSwapchain());
	logResult("create swapchain imageview", vulkanEnv.createSwapchainImageView());
	logResult("create msaa color buffer", vulkanEnv.createMsaaColorBuffer());
	logResult("create depth buffer", vulkanEnv.createDepthBuffer());
	logResult("create render pass", vulkanEnv.createRenderPass());
	logResult("create descriptor set layout", vulkanEnv.createDescriptorSetLayout());
	logResult("create graphics pipeline layout", vulkanEnv.createGraphicsPipelineLayout());
	logResult("create graphics pipeline", vulkanEnv.createGraphicsPipeline());
	logResult("create frame buffer", vulkanEnv.createFrameBuffer());
	logResult("setup fence", vulkanEnv.setupFence());
	logResult("create command pool", vulkanEnv.createCommandPool());
	logResult("create texture image", vulkanEnv.createTextureImage(imageInput));
	logResult("create texture image view", vulkanEnv.createTextureImageView());
	logResult("create texture sampler", vulkanEnv.createTextureSampler());
	logResult("create vertex/index buffer", vulkanEnv.createVertexBufferIndice(renderingData.getRenderList()));
	logResult("create uniform buffer", vulkanEnv.createUniformBuffer());
	logResult("create descriptor pool", vulkanEnv.createDescriptorPool());
	logResult("create descriptor set", vulkanEnv.createDescriptorSet());
	logResult("allocate swapchain command buffer", vulkanEnv.allocateFrameCommandBuffer());
	logResult("create frame sync object", vulkanEnv.createFrameSyncObject());
	logResult("update uniform buffer", vulkanEnv.updateUniformBuffer());
	std::cout << std::endl;

	windowLayer.setEventCallback(&renderContext, onFramebufferResize);

	while (!windowLayer.shouldClose()) {
		windowLayer.handleEvent();
		modelList[0].animate(90);
		modelList[1].animate(45);
		vulkanEnv.drawFrame(renderingData);
	}

	vulkanEnv.waitUntilIdle();
	vulkanEnv.destroy();

	windowLayer.destroy();

	return 0;
}