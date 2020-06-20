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
	renderContext->vulkanEnv->getSwapchain().onFramebufferResize();
	if (height > 0) {
		renderContext->renderingData->setAspectRatio(width / (float)height);
	}
}

void onKeyPressed(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS) {
		auto* renderContext = reinterpret_cast<RenderingTest::RenderContext*>(glfwGetWindowUserPointer(window));
		auto& option = renderContext->renderingData->getDebugOption();
		//value for shader side debug
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
			std::cout << "metalness=" << option.y << std::endl;
		}
		if (key == GLFW_KEY_K) {
			option.y = std::min(1.0f, option.y + 0.1f);
			std::cout << "metalness=" << option.y << std::endl;
		}
		//roughness
		if (key == GLFW_KEY_N) {
			option.z = std::max(0.1f, option.z - 0.1f);
			std::cout << "roughness=" << option.z << std::endl;
		}
		if (key == GLFW_KEY_M) {
			option.z = std::min(1.0f, option.z + 0.1f);
			std::cout << "roughness=" << option.z << std::endl;
		}
		//debug only, alway update
		renderContext->vulkanEnv->updateUniformBuffer();
	}
}


 void RenderingTest::prepareModel(const Setting::Misc& input) {
	 glm::vec3 noNormal(0.0f);
	 VertexIndexed tetrahedron = {
		 {
			 //position, normal, color, uv
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
			 //position, normal, color, uv
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

 std::vector<MeshRenderData> RenderingTest::setupRenderList() {
	 auto& meshList = meshManager.getMeshList();
	 std::vector<MeshRenderData> meshRenderList(meshList.size());
	 for (auto i = 0; i < meshList.size(); ++i) {
		 auto& mesh = meshList[i];
		 meshRenderList[i] = {
			 &mesh,
			 &materialManager.getMaterial(mesh.getMaterialIndex())
		 };
	 }
	 return meshRenderList;
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
	//create a test texture, with runtime generated mipmap
	ImageInput defaultTexture;
	defaultTexture.setPreserved(true);
	defaultTexture.setMipLevel(3);
	//texture loading test
	logResult("texture loading", defaultTexture.load(setting.misc.texturePath));
	textureManager.addTexture(std::move(defaultTexture));
	//defaut shaders
	ShaderInput defaultShader(setting.misc.vertexShaderPath, setting.misc.fragmentShaderPath);
	shaderManager.addShader(std::move(defaultShader));
	//default material(s)
	MaterialInput defaultMaterial;
	defaultMaterial.addTextureEntry(0);
	materialManager.addMaterial(std::move(defaultMaterial));

	prepareModel(setting.misc);
	renderingData.updateCamera(45.0f, WIDTH / (float)HEIGHT, glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	renderingData.addLight({ LightType::Point, 1.0f, 5.0f, glm::vec3(4.0f, -4.0f, 4.0f) });
	renderingData.updateLight();
	renderingData.setDebugOption({ 0.0f, 1.0f, 0.1f, 0.0f });
	renderingData.setRenderListFiltered(setupRenderList(), materialManager, textureManager);
	
	auto& swapchain = vulkanEnv.getSwapchain();
	swapchain.setWindow(windowLayer.getWindow());
	swapchain.setMaxFrameInFlight(graphicsSetting.MaxFrameInFlight);
	swapchain.setMsaaSample(graphicsSetting.MSAASample);

	vulkanEnv.setRenderingData(renderingData);
	vulkanEnv.setRenderingManager(materialManager, shaderManager);
	if (setting.misc.enableValidationLayer) {
		vulkanEnv.enableValidationLayer({ "VK_LAYER_KHRONOS_validation" });
	}
	vulkanEnv.checkExtensionRequirement();

	//render context used for C-style callback methods
	renderContext.vulkanEnv = &vulkanEnv;
	renderContext.renderingData = &renderingData;
	windowLayer.setUserDataPtr(&renderContext);

	//initialization sequence
	logResult("create instance", vulkanEnv.createInstance(APP_TITLE));
	logResult("create surface", swapchain.createSurface());
	logResult("create physical device", vulkanEnv.createPhysicalDevice());
	logResult("create logical device", vulkanEnv.createDevice());
	logResult("create swapchain", swapchain.createSwapchain());
	logResult("create allocator", vulkanEnv.createAllocator());
	logResult("create msaa color buffer", swapchain.createMsaaColorBuffer());
	logResult("create depth buffer", swapchain.createDepthBuffer());
	logResult("create render pass", vulkanEnv.createRenderPass());
	logResult("create descriptor set layout", vulkanEnv.createDescriptorSetLayout());
	logResult("create graphics pipeline layout", vulkanEnv.createGraphicsPipelineLayout());
	logResult("loading shader", shaderManager.preload());
	logResult("create graphics pipeline", vulkanEnv.createGraphicsPipeline());
	shaderManager.unload();
	logResult("create frame buffer", vulkanEnv.createFramebuffer());
	logResult("setup fence", vulkanEnv.setupFence());
	logResult("create command pool", vulkanEnv.createCommandPool());
	logResult("create texture image", vulkanEnv.createTextureImage(textureManager.getTextureList()));
	textureManager.releaseNonPreserved();
	logResult("create texture image view", vulkanEnv.createTextureImageView());
	logResult("create texture sampler", vulkanEnv.createTextureSampler());
	logResult("create vertex/index buffer", vulkanEnv.createVertexBufferIndice());
	logResult("create uniform buffer", vulkanEnv.createUniformBuffer());
	logResult("prepare descriptor", vulkanEnv.prepareDescriptor());
	logResult("allocate swapchain command buffer", vulkanEnv.allocateFrameCommandBuffer());
	logResult("create frame sync object", vulkanEnv.createFrameSyncObject());
	logResult("update uniform buffer", vulkanEnv.updateUniformBuffer());
	//flush output
	std::cout << std::endl;

	//glfw event callback
	windowLayer.setEventCallback(onFramebufferResize);
	windowLayer.setKeyCallback(onKeyPressed);

	//render loop
	while (!windowLayer.shouldClose()) {
		//frame dependent input handling
		windowLayer.handleEvent();

		//animate mesh forr debugging
		//meshManager.getMeshAt(0).animate(90);
		//meshManager.getMeshAt(1).animate(45);
		meshManager.getMeshAt(0).animate(15);

		if (!vulkanEnv.drawFrame(renderingData)) {
			//draw frame failed, only consecutive failure causes loop exit
			if (++drawFailure > DRAW_FAILURE_THRESHOLD) {
				std::cout << "Consecutive frame draw failure! (" << drawFailure << ")"  << std::endl;
				break;
			}
			else {
				drawFailure = 0;
			}
		}
	}

	//cleanup
	vulkanEnv.waitUntilIdle();
	vulkanEnv.destroy();

	windowLayer.destroy();

	return 0;
}