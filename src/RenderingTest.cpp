#include "RenderingTest.h"
#include "ModelImport.h"
#include "ShaderInput.h"
#include "DebugHelper.hpp"
#include "MeshNode.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <algorithm>

const char* APP_TITLE = "vulkan";
constexpr uint32_t WIDTH = 1200;
constexpr uint32_t HEIGHT = 1200;
constexpr int DRAW_FAILURE_THRESHOLD = 120;

void onFramebufferResize(GLFWwindow* window, int width, int height) {
	auto* renderContext = reinterpret_cast<RenderingTest::RenderContext*>(glfwGetWindowUserPointer(window));
	renderContext->vulkanEnv->onFramebufferResize();
	if (height > 0) {
		renderContext->renderingData->setAspectRatio(width / (float)height);
	}
}

void onKeyPressed(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS) {
		auto* renderContext = reinterpret_cast<RenderingTest::RenderContext*>(glfwGetWindowUserPointer(window));
		auto& option = renderContext->renderingData->getDebugOption();
		if (key == GLFW_KEY_P) {
			option.x = 0;
		}
		if (key == GLFW_KEY_Q) {
			option.x = 1;
		}
		if (key == GLFW_KEY_W) {
			option.x = 2;
		}
		if (key == GLFW_KEY_E) {
			option.x = 3;
		}
		if (key == GLFW_KEY_R) {
			option.x = 4;
		}
		//metalness
		if (key == GLFW_KEY_J) {
			option.y = std::max(0.0f, option.y - 0.1f);
		}
		if (key == GLFW_KEY_K) {
			option.y = std::min(1.0f, option.y + 0.1f);
		}
		//roughness
		if (key == GLFW_KEY_N) {
			option.z = std::max(0.1f, option.z - 0.1f);
		}
		if (key == GLFW_KEY_M) {
			option.z = std::min(1.0f, option.z + 0.1f);
		}
		//debug only, alway update
		renderContext->vulkanEnv->updateUniformBuffer();
	}
}


 void RenderingTest::prepareModel(const Setting::Misc& input) {
	 glm::vec3 noNormal(0.0f);
	 VertexIndexed tetrahedron = {
		 {
			 {{0.0f, -0.577f, 0.0f}, noNormal, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
			 {{0.0f, 0.289f, 0.577f}, noNormal, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
			 {{-0.5f, 0.289f, -0.289f}, noNormal, {0.0f, 1.0f, 0.0f}, {0.5f, 0.0f}},
			 {{0.5f, 0.289f, -0.289f}, noNormal, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}
		 },
		 {
			 0, 1, 2,
			 0, 2, 3,
			 0, 3, 1,
			 3, 2, 1,
		 },
		 0
	 };
	 MeshInput inputTetrahedron{ { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f, 0.0f }, {1.0f, 1.0f, 1.0f} };
	 inputTetrahedron.calculateNormal(tetrahedron);
	 inputTetrahedron.setMesh(std::move(tetrahedron));
	 VertexIndexed cube = {
		 {
			 {{-0.5f, -0.5f, 0.5f}, noNormal, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
			 {{0.5f, -0.5f, 0.5f}, noNormal, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
			 {{0.5f, 0.5f, 0.5f}, noNormal, {0.0f, 1.0f, 0.0f}, {0.5f, 0.0f}},
			 {{-0.5f, 0.5f, 0.5f}, noNormal, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
			 {{-0.5f, -0.5f, -0.5f}, noNormal, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
			 {{-0.5f, 0.5f, -0.5f}, noNormal, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
			 {{0.5f, 0.5f, -0.5f}, noNormal, {0.0f, 1.0f, 0.0f}, {0.5f, 0.0f}},
			 {{0.5f, -0.5f, -0.5f}, noNormal, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		 },
		 {
			 4, 5, 6, 6, 7, 4,//back
			 0, 1, 2, 2, 3, 0,//front
			 4, 0, 3, 3, 5, 4,//left
			 1, 7, 6, 6, 2, 1,//right
			 5, 3, 2, 2, 6, 5,//bottom
			 0, 4, 7, 7, 1, 0,//top
		 },
		 0
	 };
	 MeshInput inputCube{ { 0.0f, 0.0f, -2.0f }, { 1.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } };
	 inputCube.calculateNormal(cube);
	 inputCube.setMesh(std::move(cube));
	 MeshInput inputLoadedModel{ { 0.0f, 0.0f, 0.0f }, glm::quat({0.0f, glm::radians(-45.0f), glm::radians(180.0f)}), {1.0f, 1.0f, 1.0f} };
	 ModelImport modelImport;
	 logResult("model loading", modelImport.load(input.modelPath, { 1.5, inputLoadedModel, textureManager, materialManager }));
	 //meshManager.addMesh(std::move(inputTetrahedron));
	 //meshManager.addMesh(std::move(inputCube));
	 meshManager.addMesh(std::move(inputLoadedModel));
 }

int RenderingTest::mainLoop() {
	if (!setting.loadFrom("_input")) {
		return 2;
	}

	if (!windowLayer.init()) {
		return 1;
	}
	windowLayer.createWindow(APP_TITLE, WIDTH, HEIGHT);

	const auto& graphicsSetting = setting.graphics;
	ImageInput defaultTexture(true, true);
	defaultTexture.setMipLevel(3);
	logResult("texture loading", defaultTexture.load(setting.misc.texturePath));
	textureManager.addTexture(std::move(defaultTexture));
	ShaderInput defaultShader(setting.misc.vertexShaderPath, setting.misc.fragmentShaderPath);
	shaderManager.addShader(std::move(defaultShader));
	MaterialInput defaultMaterial;
	defaultMaterial.addTextureEntry(0);
	materialManager.addMaterial(std::move(defaultMaterial));

	prepareModel(setting.misc);
	renderingData.updateCamera(45.0f, WIDTH / (float)HEIGHT, glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	renderingData.addLight({ LightType::Point, 1.0f, 5.0f, glm::vec3(4.0f, -4.0f, 4.0f) });
	renderingData.updateLight();
	renderingData.setRenderListFiltered(meshManager.getMeshList());
	renderingData.setDebugOption({ 0.0f, 1.0f, 0.1f, 0.0f });
	
	vulkanEnv.setWindow(windowLayer.getWindow());
	vulkanEnv.setRenderingData(renderingData);
	vulkanEnv.setRenderingManager(materialManager, shaderManager);
	if (setting.misc.enableValidationLayer) {
		vulkanEnv.enableValidationLayer({ "VK_LAYER_KHRONOS_validation" });
	}
	vulkanEnv.setMaxFrameInFlight(graphicsSetting.MaxFrameInFlight);
	vulkanEnv.setMsaaSample(graphicsSetting.MSAASample);
	renderContext.vulkanEnv = &vulkanEnv;
	renderContext.renderingData = &renderingData;
	renderContext.shaderManager = &shaderManager;
	windowLayer.setUserDataPtr(&renderContext);

	logResult("create instance", vulkanEnv.createInstance(APP_TITLE));
	logResult("create surface", vulkanEnv.createSurface());
	logResult("create physical device", vulkanEnv.createPhysicalDevice());
	logResult("create logical device", vulkanEnv.createDevice());
	logResult("create allocator", vulkanEnv.createAllocator());
	logResult("create swapchain", vulkanEnv.createSwapchain());
	logResult("create swapchain imageview", vulkanEnv.createSwapchainImageView());
	logResult("create msaa color buffer", vulkanEnv.createMsaaColorBuffer());
	logResult("create depth buffer", vulkanEnv.createDepthBuffer());
	logResult("create render pass", vulkanEnv.createRenderPass());
	logResult("create descriptor set layout", vulkanEnv.createDescriptorSetLayout());
	logResult("create graphics pipeline layout", vulkanEnv.createGraphicsPipelineLayout());
	logResult("loading shader", shaderManager.preload());
	logResult("create graphics pipeline", vulkanEnv.createGraphicsPipeline());
	shaderManager.unload();
	logResult("create frame buffer", vulkanEnv.createFrameBuffer());
	logResult("setup fence", vulkanEnv.setupFence());
	logResult("create command pool", vulkanEnv.createCommandPool());
	logResult("create texture image", vulkanEnv.createTextureImage(textureManager.getTextureList()));
	textureManager.releaseNonPreserved();
	logResult("create texture image view", vulkanEnv.createTextureImageView());
	logResult("create texture sampler", vulkanEnv.createTextureSampler());
	logResult("create vertex/index buffer", vulkanEnv.createVertexBufferIndice(renderingData.getRenderList()));
	logResult("create uniform buffer", vulkanEnv.createUniformBuffer());
	logResult("prepare descriptor", vulkanEnv.prepareDescriptor());
	logResult("allocate swapchain command buffer", vulkanEnv.allocateFrameCommandBuffer());
	logResult("create frame sync object", vulkanEnv.createFrameSyncObject());
	logResult("update uniform buffer", vulkanEnv.updateUniformBuffer());
	std::cout << std::endl;

	windowLayer.setEventCallback(onFramebufferResize);
	windowLayer.setKeyCallback(onKeyPressed);

	while (!windowLayer.shouldClose()) {
		windowLayer.handleEvent();
		//meshManager.getMeshAt(0).animate(90);
		//meshManager.getMeshAt(1).animate(45);
		meshManager.getMeshAt(0).animate(15);
		if (!vulkanEnv.drawFrame(renderingData)) {
			if (++drawFailure > DRAW_FAILURE_THRESHOLD) {
				std::cout << "Consecutive frame draw failure! (" << drawFailure << ")"  << std::endl;
				break;
			}
			else {
				drawFailure = 0;
			}
		}
	}

	vulkanEnv.waitUntilIdle();
	vulkanEnv.destroy();

	windowLayer.destroy();

	return 0;
}