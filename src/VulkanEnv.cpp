#include "VulkanEnv.h"
#include "MeshNode.h"
#include "MeshInput.h"
#include "DebugHelper.hpp"
#include "VulkanHelper.h"
#include <unordered_set>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <array>

constexpr int TestMaxTextureCount = 5;

VulkanSwapchain& VulkanEnv::getSwapchain() noexcept {
	return swapchain;
}

void VulkanEnv::setRenderingData(const RenderingData& data) noexcept {
	renderingData = &data;
}

void VulkanEnv::setRenderingManager(const MaterialManager& material, ShaderManager& shader) noexcept {
	materialManager = &material;
	shaderManager = &shader;
}

void VulkanEnv::enableValidationLayer(std::vector<const char*>&& layer) {
	validationLayer = std::move(layer);
}

void VulkanEnv::checkExtensionRequirement() {
	extension = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	optionalExtensionOffset = static_cast<uint32_t>(extension.size());
	//TODO add optional
	/*
	extension.reserve(extension.size() + count);
	extension.push_back(...);
	*/
}

void VulkanEnv::waitUntilIdle() {
	vkDeviceWaitIdle(device);
}

bool VulkanEnv::createInstance(const char* appName) {
	VkApplicationInfo appInfo;
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr;
	appInfo.pApplicationName = appName;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "None";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2;

	VkInstanceCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	info.pNext = nullptr;
	info.pApplicationInfo = &appInfo;
	info.ppEnabledExtensionNames = glfwGetRequiredInstanceExtensions(&info.enabledExtensionCount);
	info.enabledLayerCount = static_cast<uint32_t>(validationLayer.size());
	info.ppEnabledLayerNames = validationLayer.data();

	if (vkCreateInstance(&info, nullptr, &instance) != VK_SUCCESS) {
		return false;
	}
	swapchain.setInstance(instance);
	return true;
}

bool VulkanEnv::queueFamilyValid(const VkPhysicalDevice device, uint32_t& score) {
	uint32_t count;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
	if (count == 0) return false;
	std::vector<VkQueueFamilyProperties> properties(count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &count, properties.data());

	bool result = false;
	int i = 0;
	for (const auto& queue : properties) {
		if (queue.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			queueFamily.graphics = i;
			std::cout << "execute queue family " << i << std::endl;
			result = true;
			break;
		}
		++i;
	}
	VkBool32 supportPresent = false;
	i = 0;
	for (const auto& queue : properties) {
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, swapchain.getSurface(), &supportPresent);
		if (supportPresent) {
			queueFamily.present = i;
			std::cout << "present queue family " << i << std::endl;
			result &= true;
			break;
		}
		++i;
	}
	score += static_cast<uint32_t>(PhysicalDeviceScore::QueueFamilyValid);
	return result;
}

bool VulkanEnv::createPhysicalDevice() {
	uint32_t count;
	vkEnumeratePhysicalDevices(instance, &count, nullptr);
	if (count == 0) return false;
	std::vector<VkPhysicalDevice> deviceList(count);
	vkEnumeratePhysicalDevices(instance, &count, deviceList.data());

	SwapchainSupport support;
	uint32_t score;
	uint32_t maxScore = 0;
	size_t deviceIndex = 0;
	for (const auto& device : deviceList) {
		if (!querySwapChainSupport(device, swapchain.getSurface(), &support)) {
			continue;
		}
		score = 0;
		if (deviceValid(device, score) &&
			queueFamilyValid(device, score) &&
			deviceExtensionSupport(device, extension, optionalExtensionOffset, score) &&
			deviceFeatureSupport(device, score)) {
			std::cout << "device candidate score=" << score << std::endl;
			if (score > maxScore) {
				maxScore = score;
				deviceIndex = physicalDeviceCandidate.size();
			}
			physicalDeviceCandidate.push_back({ device, support, score });
		}
	}
	if (physicalDeviceCandidate.size() == 0) return false;
	selectPhysicalDevice(physicalDeviceCandidate[deviceIndex]);
	return true;
}

void VulkanEnv::selectPhysicalDevice(const PhysicalDeviceCandidate& candidate) {
	swapchain.selectPhysicalDevice(candidate);
	physicalDevice = candidate.device;
}

bool VulkanEnv::createDevice() {
	std::unordered_set<uint32_t> uniqueQueueFamily{ queueFamily.graphics, queueFamily.present };
	std::vector<VkDeviceQueueCreateInfo> queueCreate;
	queueCreate.reserve(uniqueQueueFamily.size());

	for (const auto family : uniqueQueueFamily) {
		VkDeviceQueueCreateInfo queueInfo;
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.flags = 0;
		queueInfo.pNext = nullptr;
		queueInfo.queueFamilyIndex = family;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &queuePriority;
		queueCreate.push_back(queueInfo);
	}

	VkPhysicalDeviceFeatures features{};
	features.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	info.pQueueCreateInfos = queueCreate.data();
	info.queueCreateInfoCount = static_cast<uint32_t>(queueCreate.size());
	info.pEnabledFeatures = &features;
	info.enabledLayerCount = static_cast<uint32_t>(validationLayer.size());
	info.ppEnabledLayerNames = validationLayer.data();
	info.enabledExtensionCount = static_cast<uint32_t>(extension.size());
	info.ppEnabledExtensionNames = extension.data();

	if (vkCreateDevice(physicalDevice, &info, nullptr, &device) != VK_SUCCESS) {
		return false;
	}
	swapchain.setDevice(device);
	vkGetDeviceQueue(device, queueFamily.graphics, 0, &graphicsQueue);
	vkGetDeviceQueue(device, queueFamily.present, 0, &presentQueue);
	return true;
}

bool VulkanEnv::createAllocator() {
	VmaAllocatorCreateInfo info{};
	//max frame count - 1
	info.frameInUseCount = swapchain.size() - 1;
	info.instance = instance;
	info.physicalDevice = physicalDevice;
	info.device = device;
	if (vmaCreateAllocator(&info, &vmaAllocator) != VK_SUCCESS) {
		return false;
	}
	swapchain.setAllocator(vmaAllocator);
	return true;
}

bool VulkanEnv::createRenderPass() {
	auto msaaSample = swapchain.msaaSampleCount();

	VkAttachmentDescription colorAttachment;
	colorAttachment.flags = 0;
	colorAttachment.format = swapchain.getFormat();
	colorAttachment.samples = msaaSample;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;//don't care what it was previously
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	VkAttachmentReference colorAttachmentRef;
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment;
	depthAttachment.flags = 0;
	depthAttachment.format = swapchain.depthFormat();
	depthAttachment.samples = msaaSample;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	VkAttachmentReference depthAttachmentRef;
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass;
	subpass.flags = 0;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;//index of this maps to the "layout(location = 0) out" variable in shader code
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	subpass.pResolveAttachments = nullptr;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = nullptr;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = nullptr;

	VkSubpassDependency dependency;
	dependency.dependencyFlags = 0;
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo;
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.flags = 0;
	renderPassInfo.pNext = nullptr;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VkAttachmentDescription attachments[3];
	renderPassInfo.pAttachments = attachments;
	if (msaaSample != VK_SAMPLE_COUNT_1_BIT) {
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachmentRef.attachment = 2;
		VkAttachmentDescription colorResolve;
		colorResolve.flags = 0;
		colorResolve.format = swapchain.getFormat();
		colorResolve.samples = VK_SAMPLE_COUNT_1_BIT;
		colorResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		colorResolve.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		VkAttachmentReference colorResolveRef;
		colorResolveRef.attachment = 0;
		colorResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		subpass.pResolveAttachments = &colorResolveRef;

		attachments[0] = colorResolve;
		attachments[1] = depthAttachment;
		attachments[2] = colorAttachment;
		renderPassInfo.attachmentCount = 3;
	}
	else {
		attachments[0] = colorAttachment;
		attachments[1] = depthAttachment;
		renderPassInfo.attachmentCount = 2;
	}

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		return false;
	}
	swapchain.setRenderPass(renderPass);
	return true;
}

bool VulkanEnv::createDescriptorSetLayout() {
	auto& prototypeList = renderingData->getPrototypeList();
	//TODO create descriptor set for each prototype

	VkDescriptorSetLayoutBinding uniformMatrix;
	uniformMatrix.binding = 0;
	uniformMatrix.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformMatrix.descriptorCount = 1;
	uniformMatrix.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uniformMatrix.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding uniformLight;
	uniformLight.binding = 1;
	uniformLight.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformLight.descriptorCount = 1;
	uniformLight.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	uniformLight.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding sampler;
	sampler.binding = 2;
	sampler.descriptorCount = 1;
	sampler.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	sampler.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	sampler.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding binding[]{ uniformMatrix, uniformLight, sampler };
	VkDescriptorSetLayoutCreateInfo uniformInfo;
	uniformInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	uniformInfo.flags = 0;
	uniformInfo.pNext = nullptr;
	uniformInfo.bindingCount = 3;
	uniformInfo.pBindings = binding;
	if (vkCreateDescriptorSetLayout(device, &uniformInfo, nullptr, &descriptorSetLayoutUniform) != VK_SUCCESS) {
		return false;
	}

	const auto& matPrototypeList = materialManager->getPrototypeList();
	descriptorSetLayoutMaterial.resize(matPrototypeList.size());
	for (auto k = 0; k < matPrototypeList.size(); ++k) {
		const auto& prototype = matPrototypeList[k];
		//TODO prototype.textureCount;
		std::vector<VkDescriptorSetLayoutBinding> binding(TestMaxTextureCount + 1);
		VkDescriptorSetLayoutBinding& value = binding[0];
		value.binding = 0;
		value.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		value.descriptorCount = 1;
		value.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		value.pImmutableSamplers = nullptr;

		for (auto n = 0; n < TestMaxTextureCount; ++n) {
			auto m = n + 1;
			VkDescriptorSetLayoutBinding& texture = binding[m];
			texture.binding = m;
			texture.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			texture.descriptorCount = 1;
			texture.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			texture.pImmutableSamplers = nullptr;
		}

		VkDescriptorSetLayoutCreateInfo materialInfo;
		materialInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		materialInfo.flags = 0;
		materialInfo.pNext = nullptr;
		materialInfo.bindingCount = static_cast<uint32_t>(binding.size());
		materialInfo.pBindings = binding.data();
		if (vkCreateDescriptorSetLayout(device, &materialInfo, nullptr, &descriptorSetLayoutMaterial[k]) != VK_SUCCESS) {
			return false;
		}
	}
	return true;
}

bool VulkanEnv::createGraphicsPipelineLayout() {
	//TODO multiple layout
	VkDescriptorSetLayout layout[]{ descriptorSetLayoutUniform, descriptorSetLayoutMaterial[0] };
	if (!createGraphicsPipelineLayoutWithVertexConst(device, layout, 2, &graphicsPipelineLayout)) {
		return false;
	}
	swapchain.setGraphicsPipelineLayout(graphicsPipelineLayout);
	return true;
}

bool VulkanEnv::createGraphicsPipeline() {
	//TODO
	auto& shader = shaderManager->getShaderAt(0);
	VkShaderModule vertShader;
	createShaderModule(device, shader.getVertData(), &vertShader);
	VkShaderModule fragShader;
	createShaderModule(device, shader.getFragData(), &fragShader);

	VkPipelineShaderStageCreateInfo vertStage;
	vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertStage.flags = 0;
	vertStage.pNext = nullptr;
	vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertStage.module = vertShader;
	vertStage.pName = "main";
	vertStage.pSpecializationInfo = nullptr;
	VkPipelineShaderStageCreateInfo fragStage;
	fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragStage.flags = 0;
	fragStage.pNext = nullptr;
	fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragStage.module = fragShader;
	fragStage.pName = "main";
	fragStage.pSpecializationInfo = nullptr;
	VkPipelineShaderStageCreateInfo shaderStageInfo[] = { vertStage, fragStage };

	auto vertexBinding = MeshNode::getBindingDescription();
	auto vertexAttribute = MeshNode::getAttributeDescription();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo;
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.flags = 0;
	vertexInputInfo.pNext = nullptr;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &vertexBinding;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttribute.size());
	vertexInputInfo.pVertexAttributeDescriptions = vertexAttribute.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.flags = 0;
	inputAssemblyInfo.pNext = nullptr;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	auto& extent = swapchain.getExtent();

	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(extent.width);
	viewport.height = static_cast<float>(extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor;
	scissor.offset = { 0, 0 };
	scissor.extent = extent;

	VkPipelineViewportStateCreateInfo viewportStateInfo;
	viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateInfo.flags = 0;
	viewportStateInfo.pNext = nullptr;
	viewportStateInfo.viewportCount = 1;
	viewportStateInfo.pViewports = &viewport;
	viewportStateInfo.scissorCount = 1;
	viewportStateInfo.pScissors = &scissor;

	VkPipelineDepthStencilStateCreateInfo depthStencil;
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.flags = 0;
	depthStencil.pNext = nullptr;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {};
	depthStencil.back = {};

	VkPipelineRasterizationStateCreateInfo rasterizationInfo;
	rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationInfo.flags = 0;
	rasterizationInfo.pNext = nullptr;
	rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizationInfo.depthBiasEnable = VK_FALSE;
	rasterizationInfo.depthBiasClamp = 0.0f;
	rasterizationInfo.depthBiasConstantFactor = 0.0f;
	rasterizationInfo.depthBiasSlopeFactor = 0.0f;
	rasterizationInfo.depthClampEnable = VK_FALSE;
	rasterizationInfo.lineWidth = 1.0f;
	rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampleInfo;
	multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleInfo.flags = 0;
	multisampleInfo.pNext = nullptr;
	multisampleInfo.sampleShadingEnable = VK_FALSE;
	multisampleInfo.rasterizationSamples = swapchain.msaaSampleCount();
	multisampleInfo.minSampleShading = 1.0f;
	multisampleInfo.pSampleMask = nullptr;
	multisampleInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleInfo.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlendStateInfo;
	colorBlendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateInfo.flags = 0;
	colorBlendStateInfo.pNext = nullptr;
	colorBlendStateInfo.logicOpEnable = VK_FALSE;
	colorBlendStateInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendStateInfo.attachmentCount = 1;
	colorBlendStateInfo.pAttachments = &colorBlendAttachment;
	colorBlendStateInfo.blendConstants[0] = 0.0f;
	colorBlendStateInfo.blendConstants[1] = 0.0f;
	colorBlendStateInfo.blendConstants[2] = 0.0f;
	colorBlendStateInfo.blendConstants[3] = 0.0f;

	std::vector<VkDynamicState> dynamicState{
		VK_DYNAMIC_STATE_VIEWPORT,
	};
	VkPipelineDynamicStateCreateInfo dynamicStateInfo;
	dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateInfo.flags = 0;
	dynamicStateInfo.pNext = nullptr;
	dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicState.size());
	dynamicStateInfo.pDynamicStates = dynamicState.data();

	VkGraphicsPipelineCreateInfo pipelineInfo;
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.flags = 0;
	pipelineInfo.pNext = nullptr;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStageInfo;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineInfo.pTessellationState = nullptr;
	pipelineInfo.pRasterizationState = &rasterizationInfo;
	pipelineInfo.pViewportState = &viewportStateInfo;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pMultisampleState = &multisampleInfo;
	pipelineInfo.pColorBlendState = &colorBlendStateInfo;
	pipelineInfo.pDynamicState = &dynamicStateInfo;
	pipelineInfo.layout = graphicsPipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(
		device,
		VK_NULL_HANDLE,
		1,
		&pipelineInfo,
		nullptr,
		&graphicsPipeline) != VK_SUCCESS) {
		return false;
	}

	vkDestroyShaderModule(device, vertShader, nullptr);
	vkDestroyShaderModule(device, fragShader, nullptr);
	swapchain.setGraphicsPipeline(graphicsPipeline);
	return true;
}

bool VulkanEnv::createTextureImage(const std::vector<ImageInput>& textureList) {
	//TODO batch submit
	for (auto& texture : textureList) {
		if (!texture.isValid()) {
			continue;
		}
		auto size = texture.getByteSize();
		VkBuffer stagingBuffer;
		VmaAllocation stagingBufferAllocation;
		if (!createStagingBuffer(vmaAllocator, size, stagingBuffer, stagingBufferAllocation)) {
			return false;
		}

		void* buffer;
		vmaMapMemory(vmaAllocator, stagingBufferAllocation, &buffer);
		memcpy(buffer, texture.pixel(), size);
		vmaUnmapMemory(vmaAllocator, stagingBufferAllocation);

		ImageOption option = { texture.getMipLevel(), VK_FORMAT_R8G8B8A8_SRGB };
		VkImage image;
		VmaAllocation imageAllocation;
		VkImageCreateInfo info;
		info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.flags = 0;
		info.pNext = nullptr;
		info.imageType = VK_IMAGE_TYPE_2D;
		info.extent.width = texture.getWidth();
		info.extent.height = texture.getHeight();
		info.extent.depth = 1;
		info.mipLevels = option.mipLevel;
		info.arrayLayers = 1;
		info.format = option.format;
		if (texture.preserveData()) {
			info.tiling = VK_IMAGE_TILING_LINEAR;
			info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
		}
		else {
			info.tiling = VK_IMAGE_TILING_OPTIMAL;
			info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		}
		info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		if (texture.shouldGenerateMipmap()) {
			info.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		info.samples = VK_SAMPLE_COUNT_1_BIT;
		info.queueFamilyIndexCount = 0;
		info.pQueueFamilyIndices = nullptr;
		createImage(vmaAllocator, info, image, imageAllocation);
		imageSet.image.push_back(image);
		imageSet.option.push_back(std::move(option));
		imageSet.allocation.push_back(imageAllocation);

		VkCommandBuffer cmd;
		allocateCommandBuffer(commandPool, 1, &cmd);
		if (!beginCommand(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)) {
			return false;
		}
		cmdTransitionImageLayout(cmd, physicalDevice, image, option, info.initialLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		cmdCopyImage(cmd, stagingBuffer, image, texture.getWidth(), texture.getHeight(), 0);
		if (texture.shouldGenerateMipmap()) {
			cmdGenerateTextureMipmap(cmd, image, option, texture.getWidth(), texture.getHeight());
		}
		else {
			cmdTransitionImageLayout(cmd, physicalDevice, image, option, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
			return false;
		}

		vkResetFences(device, 1, &fenceImageCopy);
		submitCommand(&cmd, 1, graphicsQueue, fenceImageCopy);
		vkWaitForFences(device, 1, &fenceImageCopy, VK_TRUE, UINT64_MAX);

		vmaDestroyBuffer(vmaAllocator, stagingBuffer, stagingBufferAllocation);
	}
	return true;
}

bool VulkanEnv::createTextureImageView() {
	imageSet.view.reserve(imageSet.image.size());
	for (auto i = 0; i < imageSet.image.size(); ++i) {
		const auto& image = imageSet.image[i];
		const auto& option = imageSet.option[i];
		VkImageView view;
		if (!createImageView(device, image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, option.mipLevel, view)) {
			return false;
		}
		imageSet.view.push_back(view);
	}
	return true;
}

bool VulkanEnv::createTextureSampler() {
	VkSamplerCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	info.flags = 0;
	info.pNext = nullptr;
	info.magFilter = VK_FILTER_LINEAR;
	info.minFilter = VK_FILTER_LINEAR;
	info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	info.anisotropyEnable = VK_TRUE;
	info.maxAnisotropy = 16;
	info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	info.unnormalizedCoordinates = VK_FALSE;
	info.compareEnable = VK_FALSE;
	info.compareOp = VK_COMPARE_OP_ALWAYS;
	info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	info.mipLodBias = 0.0f;
	info.minLod = 0.0f;
	info.maxLod = static_cast<float>(imageSet.option[0].mipLevel);
	VkSampler sampler;
	if (vkCreateSampler(device, &info, nullptr, &sampler) != VK_SUCCESS) {
		return false;
	}
	imageSet.sampler.push_back(sampler);
	return true;
}

bool VulkanEnv::setupFence() {
	VkFenceCreateInfo fenceInfo;
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = 0;
	fenceInfo.pNext = nullptr;
	return vkCreateFence(device, &fenceInfo, nullptr, &fenceVertexIndexCopy) == VK_SUCCESS &&
		vkCreateFence(device, &fenceInfo, nullptr, &fenceImageCopy) == VK_SUCCESS;
}

bool VulkanEnv::createVertexBufferIndice() {
	auto& input = renderingData->getRenderList();
	uint32_t vCount = 0, vSize = 0, iSize = 0;
	for (const auto& vertexInput : input) {
		for (const auto& mesh : vertexInput->getMeshList()) {
			for (const auto& view : mesh.getView()) {
				indexBuffer.offset.push_back(iSize);
				indexBuffer.vOffset.push_back(vCount);
				indexBuffer.iCount.push_back(view.indexCount);
				indexBuffer.drawInfo.push_back(DrawInfo{ view.materialIndex, &mesh.getConstantData() });
				vSize += view.vertexSize;
				vCount += view.vertexCount;
				iSize += view.indexSize;
			}
		}
	}

	//TODO merge memory block alloc
	VkBuffer stagingVBuffer;
	VmaAllocation stagingVBufferAllocation;
	auto vBufferSuccess = createStagingBuffer(vmaAllocator, vSize, stagingVBuffer, stagingVBufferAllocation);
	VkBuffer stagingIBuffer;
	VmaAllocation stagingIBufferAllocation;
	auto iBufferSuccess = createStagingBuffer(vmaAllocator, vSize, stagingIBuffer, stagingIBufferAllocation);
	if(!vBufferSuccess || !iBufferSuccess) {
		return false;
	}

	void* vData;
	void* iData;
	vSize = 0;
	iSize = 0;
	vmaMapMemory(vmaAllocator, stagingVBufferAllocation, &vData);
	vmaMapMemory(vmaAllocator, stagingIBufferAllocation, &iData);
	for (const auto& vertexInput : input) {
		for (const auto& mesh : vertexInput->getMeshList()) {
			for (const auto& view : mesh.getView()) {
				memcpy((uint8_t*)vData + vSize, vertexInput->bufferData(view.bufferIndex) + view.vertexOffset, view.vertexSize);
				memcpy((uint8_t*)iData + iSize, vertexInput->bufferData(view.bufferIndex) + view.indexOffset, view.indexSize);
				vSize += view.vertexSize;
				iSize += view.indexSize;
			}
		}
	}
	vmaUnmapMemory(vmaAllocator, stagingVBufferAllocation);
	vmaUnmapMemory(vmaAllocator, stagingIBufferAllocation);

	VkBuffer vBuffer;
	VmaAllocation vAllocation;
	vBufferSuccess = createBuffer(vmaAllocator, vSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY, vBuffer, vAllocation);
	VkBuffer iBuffer;
	VmaAllocation iAllocation;
	iBufferSuccess = createBuffer(vmaAllocator, iSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY, iBuffer, iAllocation);
	if (!vBufferSuccess || !iBufferSuccess) {
		return false;
	}

	VkCommandBuffer copyCmd;
	if (!allocateCommandBuffer(commandPool, 1, &copyCmd)) {
		return false;
	}
	if (!beginCommand(copyCmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)) {
		return false;
	}

	VkBufferCopy vCopy;
	vCopy.srcOffset = 0;
	vCopy.dstOffset = 0;
	vCopy.size = vSize;
	vkCmdCopyBuffer(copyCmd, stagingVBuffer, vBuffer, 1, &vCopy);
	VkBufferCopy iCopy;
	iCopy.srcOffset = 0;
	iCopy.dstOffset = 0;
	iCopy.size = iSize;
	vkCmdCopyBuffer(copyCmd, stagingIBuffer, iBuffer, 1, &iCopy);

	if (vkEndCommandBuffer(copyCmd) != VK_SUCCESS) {
		return false;
	}

	if (!submitCommand(&copyCmd, 1, graphicsQueue, fenceVertexIndexCopy)) {
		return false;
	}
	if (vkWaitForFences(device, 1, &fenceVertexIndexCopy, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
		return false;
	}

	vkFreeCommandBuffers(device, commandPool, 1, &copyCmd);
	vmaDestroyBuffer(vmaAllocator, stagingVBuffer, stagingVBufferAllocation);
	vmaDestroyBuffer(vmaAllocator, stagingIBuffer, stagingIBufferAllocation);

	vertexBuffer.buffer.push_back(vBuffer);
	vertexBuffer.offset.push_back(0);
	vertexBuffer.allocation.push_back(vAllocation);
	indexBuffer.buffer = iBuffer;
	indexBuffer.allocation = iAllocation;
	return true;
}

bool VulkanEnv::createUniformBuffer() {
	uniformBufferMatrix.resize(swapchain.size());
	uniformBufferLight.resize(swapchain.size());
	for (uint32_t i = 0; i < swapchain.size(); ++i) {
		if (!createBuffer(vmaAllocator, sizeof(MatrixUniformBufferData),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VMA_MEMORY_USAGE_CPU_TO_GPU, uniformBufferMatrix[i].buffer, uniformBufferMatrix[i].allocation) ||
			!createBuffer(vmaAllocator, sizeof(LightUniformBufferData),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VMA_MEMORY_USAGE_CPU_TO_GPU, uniformBufferLight[i].buffer, uniformBufferLight[i].allocation)) {
			return false;
		}
	}
	//TODO merge
	swapchain.copyToBufferList(uniformBufferMatrix);
	swapchain.copyToBufferList(uniformBufferLight);
	return true;
}

bool VulkanEnv::prepareDescriptor() {
	descriptorPool.resize(swapchain.size());
	descriptorPoolFree.reserve(swapchain.size());
	descriptorSet.resize(swapchain.size());
	for (auto& pool : descriptorPool) {
		if (!createDescriptorPool(0, pool)) {
			return false;
		}
		descriptorPoolFree.push_back(pool);
	}
	return true;
}

bool VulkanEnv::createDescriptorPool(int requirement, VkDescriptorPool& pool) {
	uint32_t imageCount = static_cast<uint32_t>(imageSet.image.size());//TODO this assumes no empty/unset material texture
	uint32_t materialCount = static_cast<uint32_t>(descriptorSetLayoutMaterial.size());
	uint32_t imageSetCount = materialCount * TestMaxTextureCount;//TODO
	//these determines the pool capacity
	std::array<VkDescriptorPoolSize, 3> poolSize;
	poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize[0].descriptorCount = 2 + materialCount;//matrix + light + per material
	poolSize[1].type = VK_DESCRIPTOR_TYPE_SAMPLER;
	poolSize[1].descriptorCount = 1;
	poolSize[2].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	poolSize[2].descriptorCount = imageSetCount;

	VkDescriptorPoolCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	info.flags = 0;
	info.pNext = nullptr;
	info.poolSizeCount = static_cast<uint32_t>(poolSize.size());
	info.pPoolSizes = poolSize.data();
	//this limits the set count can be allocated
	info.maxSets = imageSetCount + 2;

	return vkCreateDescriptorPool(device, &info, nullptr, &pool) == VK_SUCCESS;
}	 

bool VulkanEnv::setupDescriptorSet(int imageIndex, VkDescriptorPool pool) {
	uint32_t materialLayoutCount = static_cast<uint32_t>(descriptorSetLayoutMaterial.size());
	VkDescriptorSetAllocateInfo info1;
	info1.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	info1.pNext = nullptr;
	info1.descriptorPool = pool;
	info1.descriptorSetCount = 1;
	info1.pSetLayouts = &descriptorSetLayoutUniform;

	VkDescriptorSetAllocateInfo info2;
	info2.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	info2.pNext = nullptr;
	info2.descriptorPool = pool;
	info2.descriptorSetCount = materialLayoutCount;
	info2.pSetLayouts = descriptorSetLayoutMaterial.data();

	auto& descriptorSetPerSwapchain = descriptorSet[imageIndex];
	descriptorSetPerSwapchain.resize(materialLayoutCount + 1);//TODO reserve? then where to shrink it
	auto result1 = vkAllocateDescriptorSets(device, &info1, descriptorSetPerSwapchain.data() + materialLayoutCount);
	auto result2 = vkAllocateDescriptorSets(device, &info2, descriptorSetPerSwapchain.data());
	if (result1 != VK_SUCCESS || result2 != VK_SUCCESS) {
		return false;
	}

	auto maxTextureCountPerMaterial = 3;
	std::vector<VkWriteDescriptorSet> writeArr;
	writeArr.reserve(3 + materialLayoutCount * maxTextureCountPerMaterial);

	VkDescriptorBufferInfo matrixBufferInfo;
	matrixBufferInfo.buffer = uniformBufferMatrix[imageIndex].buffer;
	matrixBufferInfo.offset = 0;
	matrixBufferInfo.range = sizeof(MatrixUniformBufferData);
	VkWriteDescriptorSet uniformMatrixWrite;
	uniformMatrixWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	uniformMatrixWrite.pNext = nullptr;
	uniformMatrixWrite.dstSet = descriptorSetPerSwapchain.back();
	uniformMatrixWrite.dstBinding = 0;
	uniformMatrixWrite.dstArrayElement = 0;
	uniformMatrixWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformMatrixWrite.descriptorCount = 1;
	uniformMatrixWrite.pBufferInfo = &matrixBufferInfo;
	uniformMatrixWrite.pImageInfo = nullptr;
	uniformMatrixWrite.pTexelBufferView = nullptr;
	writeArr.push_back(std::move(uniformMatrixWrite));

	VkDescriptorBufferInfo lightBufferInfo;
	lightBufferInfo.buffer = uniformBufferLight[imageIndex].buffer;
	lightBufferInfo.offset = 0;
	lightBufferInfo.range = sizeof(LightUniformBufferData);
	VkWriteDescriptorSet uniformLightWrite;
	uniformLightWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	uniformLightWrite.pNext = nullptr;
	uniformLightWrite.dstSet = descriptorSetPerSwapchain.back();
	uniformLightWrite.dstBinding = 1;
	uniformLightWrite.dstArrayElement = 0;
	uniformLightWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformLightWrite.descriptorCount = 1;
	uniformLightWrite.pBufferInfo = &lightBufferInfo;
	uniformLightWrite.pImageInfo = nullptr;
	uniformLightWrite.pTexelBufferView = nullptr;
	writeArr.push_back(std::move(uniformLightWrite));

	VkDescriptorImageInfo samplerInfo;
	samplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	samplerInfo.imageView = 0;
	samplerInfo.sampler = imageSet.sampler[0];
	VkWriteDescriptorSet samplerWrite;
	samplerWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	samplerWrite.pNext = nullptr;
	samplerWrite.dstSet = descriptorSetPerSwapchain.back();
	samplerWrite.dstBinding = 2;
	samplerWrite.dstArrayElement = 0;
	samplerWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	samplerWrite.descriptorCount = 1;
	samplerWrite.pBufferInfo = nullptr;
	samplerWrite.pImageInfo = &samplerInfo;
	samplerWrite.pTexelBufferView = nullptr;
	writeArr.push_back(std::move(samplerWrite));

	std::vector<VkDescriptorImageInfo> imageInfoList(imageSet.image.size());
	for (auto k = 0; k < imageSet.image.size(); ++k) {
		VkDescriptorImageInfo& imageInfo = imageInfoList[k];
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = imageSet.view[k];
		imageInfo.sampler = 0;
	}

	const auto& matList = materialManager->getMaterialList();
	for (auto k = 0; k < matList.size(); ++k) {
		const auto& mat = matList[k];
		const auto& texEntry = mat.getTextureEntry();
		for (auto t = 0; t < texEntry.size(); ++t) {
			VkWriteDescriptorSet textureWrite;
			textureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			textureWrite.pNext = nullptr;
			textureWrite.dstSet = descriptorSetPerSwapchain[k];
			textureWrite.dstBinding = t + 1;
			textureWrite.dstArrayElement = 0;
			textureWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			textureWrite.descriptorCount = 1;
			textureWrite.pBufferInfo = nullptr;
			textureWrite.pImageInfo = &imageInfoList[texEntry[t].textureIndex];
			textureWrite.pTexelBufferView = nullptr;
			writeArr.push_back(std::move(textureWrite));
		}
	}

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeArr.size()), writeArr.data(), 0, nullptr);
	return true;
}

bool VulkanEnv::createCommandPool() {
	VkCommandPoolCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.flags = 0;
	info.pNext = nullptr;
	info.queueFamilyIndex = queueFamily.graphics;

	VkCommandPoolCreateInfo infoResetable;
	infoResetable.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	infoResetable.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	infoResetable.pNext = nullptr;
	infoResetable.queueFamilyIndex = queueFamily.graphics;

	return vkCreateCommandPool(device, &info, nullptr, &commandPool) == VK_SUCCESS &&
		vkCreateCommandPool(device, &infoResetable, nullptr, &commandPoolReset) == VK_SUCCESS;
}

bool VulkanEnv::allocateCommandBuffer(const VkCommandPool pool, const uint32_t count, VkCommandBuffer* cmd) {
	VkCommandBufferAllocateInfo cmdInfo;
	cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdInfo.pNext = nullptr;
	cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdInfo.commandPool = pool;
	cmdInfo.commandBufferCount = count;
	return vkAllocateCommandBuffers(device, &cmdInfo, cmd) == VK_SUCCESS;
}

bool VulkanEnv::allocateFrameCommandBuffer() {
	commandBuffer.resize(swapchain.size());
	return allocateCommandBuffer(commandPoolReset, static_cast<uint32_t>(commandBuffer.size()), commandBuffer.data());
}

bool VulkanEnv::setupCommandBuffer(const uint32_t index, const uint32_t imageIndex) {
	auto& cmd = commandBuffer[index];
	VkCommandBufferBeginInfo beginInfo;
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pNext = nullptr;
	beginInfo.pInheritanceInfo = nullptr;
	if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) {
		return false;
	}

	VkRenderPassBeginInfo renderPassBegin;
	renderPassBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBegin.pNext = nullptr;
	renderPassBegin.framebuffer = swapchain.getFramebuffer(imageIndex);
	renderPassBegin.renderPass = renderPass;
	renderPassBegin.renderArea.offset = { 0, 0 };
	renderPassBegin.renderArea.extent = swapchain.getExtent();
	VkClearValue clearColor[3];
	renderPassBegin.pClearValues = clearColor;
	if (swapchain.msaaSampleCount() == VK_SAMPLE_COUNT_1_BIT) {
		clearColor[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearColor[1].depthStencil = { 1.0f, 0 };
		renderPassBegin.clearValueCount = 2;
	}
	else {
		clearColor[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearColor[1].depthStencil = { 1.0f, 0 };
		clearColor[2].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassBegin.clearValueCount = 3;
	}

	vkCmdBeginRenderPass(cmd, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	const auto& currentDescriptorSet = descriptorSet[imageIndex];
	vkCmdBindVertexBuffers(cmd, 0, static_cast<uint32_t>(vertexBuffer.buffer.size()), vertexBuffer.buffer.data(), vertexBuffer.offset.data());
	for (auto i = 0; i < indexBuffer.offset.size(); ++i) {
		const auto& drawInfo = indexBuffer.drawInfo[i];
		VkDescriptorSet bindingSet[]{ currentDescriptorSet.back(), currentDescriptorSet[drawInfo.setIndex] };
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 0, 2, bindingSet, 0, nullptr);
		vkCmdBindIndexBuffer(cmd, indexBuffer.buffer, indexBuffer.offset[i], VK_INDEX_TYPE_UINT16);
		vkCmdPushConstants(cmd, graphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, MeshNode::getConstantSize(), drawInfo.constantData);
		vkCmdDrawIndexed(cmd, indexBuffer.iCount[i], 1, 0, indexBuffer.vOffset[i], 0);
	}
	vkCmdEndRenderPass(cmd);
	return vkEndCommandBuffer(cmd) == VK_SUCCESS;
}

bool VulkanEnv::createFrameSyncObject() {
	inFlightFrame.resize(swapchain.size());

	VkSemaphoreCreateInfo semaphoreInfo;
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.flags = 0;
	semaphoreInfo.pNext = nullptr;

	VkFenceCreateInfo fenceInfo;
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	fenceInfo.pNext = nullptr;

	for (auto& frame : inFlightFrame) {
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frame.semaphoreRenderFinished) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &frame.fenceImageAcquired) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &frame.fenceInFlight) != VK_SUCCESS) {
			return false;
		}
	}
	return true;
}

void VulkanEnv::destroy() {
	for (auto& frame : inFlightFrame) {
		vkDestroySemaphore(device, frame.semaphoreRenderFinished, nullptr);
		vkDestroyFence(device, frame.fenceImageAcquired, nullptr);
		vkDestroyFence(device, frame.fenceInFlight, nullptr);
	}
	swapchain.destroy();
	retiredSwapchain.destroy();
	for (auto& pool : descriptorPool) {
		vkDestroyDescriptorPool(device, pool, nullptr);
	}
	for (auto i = 0; i < vertexBuffer.buffer.size(); ++i) {
		vmaDestroyBuffer(vmaAllocator, vertexBuffer.buffer[i], vertexBuffer.allocation[i]);
	}
	vmaDestroyBuffer(vmaAllocator, indexBuffer.buffer, indexBuffer.allocation);
	vkDestroyFence(device, fenceVertexIndexCopy, nullptr);
	for (auto i = 0; i < imageSet.image.size(); ++i) {
		vkDestroyImageView(device, imageSet.view[i], nullptr);
		vmaDestroyImage(vmaAllocator, imageSet.image[i], imageSet.allocation[i]);
	}
	for (const auto& sampler : imageSet.sampler) {
		vkDestroySampler(device, sampler, nullptr);
	}
	vkDestroyFence(device, fenceImageCopy, nullptr);
	vkDestroyCommandPool(device, commandPool, nullptr);
	vkDestroyCommandPool(device, commandPoolReset, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayoutUniform, nullptr);
	for (auto& layout : descriptorSetLayoutMaterial) {
		vkDestroyDescriptorSetLayout(device, layout, nullptr);
	}
	vmaDestroyAllocator(vmaAllocator);
	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(instance, swapchain.getSurface(), nullptr);
	vkDestroyInstance(instance, nullptr);
}

bool VulkanEnv::recreateSwapchain() {
	for (auto& frame : inFlightFrame) {
		swapchain.copyDescriptorPool(frame.descriptorPool);
		frame.descriptorPool = nullptr;
	}
	retiredSwapchain.destroy();
	retiredSwapchain = swapchain;
	swapchain.querySupport();

	bool result = logResult("create swapchain", swapchain.createSwapchain()) &&
		logResult("create msaa color buffer", swapchain.createMsaaColorBuffer()) &&
		logResult("create depth buffer", swapchain.createDepthBuffer()) &&
		logResult("create render pass", createRenderPass()) &&
		logResult("create graphics pipeline layout", createGraphicsPipelineLayout()) &&
		logResult("create graphics pipeline", createGraphicsPipeline()) &&
		logResult("create framebuffer", swapchain.createFramebuffer()) &&
		logResult("create uniform buffer", createUniformBuffer()) &&
		logResult("prepare descriptor", prepareDescriptor()) &&
		logResult("update uniform buffer", updateUniformBuffer());
	return result;
}

bool VulkanEnv::updateUniformBuffer() {
	for (auto i = 0; i < uniformBufferMatrix.size(); ++i) {
		if (!updateUniformBufferMatrix(i) || ! updateUniformBufferLight(i)) {
			return false;
		}
	}
	return true;
}

bool VulkanEnv::updateUniformBufferMatrix(const uint32_t imageIndex) {
	const auto& data = renderingData->getMatrixUniform();
	void* buffer;
	vmaMapMemory(vmaAllocator, uniformBufferMatrix[imageIndex].allocation, &buffer);
	memcpy(buffer, &data, sizeof(data));
	vmaUnmapMemory(vmaAllocator, uniformBufferMatrix[imageIndex].allocation);
	return true;
}

bool VulkanEnv::updateUniformBufferLight(const uint32_t imageIndex) {
	const auto& data = renderingData->getLightUniform();
	void* buffer;
	vmaMapMemory(vmaAllocator, uniformBufferLight[imageIndex].allocation, &buffer);
	memcpy(buffer, &data, sizeof(data));
	vmaUnmapMemory(vmaAllocator, uniformBufferLight[imageIndex].allocation);
	return true;
}

void VulkanEnv::releaseDescriptorPool(VkDescriptorPool pool) {
	if (pool == nullptr) return;
	vkResetDescriptorPool(device, pool, 0);
	descriptorPoolFree.push_back(pool);
}

bool VulkanEnv::requestDescriptorPool(int requirement, VkDescriptorPool& pool) {
	//TODO check requirement & create if no matching one exists
	if (descriptorPoolFree.empty()) {
		return createDescriptorPool(requirement, pool);
	}
	pool = descriptorPoolFree.back();
	descriptorPoolFree.pop_back();
	return true;
}

bool VulkanEnv::frameResizeCheck(VkResult result) {
	if (swapchain.resized() || result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		swapchain.waitForValidSize();

		shaderManager->preload();
		recreateSwapchain();
		shaderManager->unload();
		return false;
	}
	if (result != VK_SUCCESS) {
		return false;
	}
	return true;
}

bool VulkanEnv::drawFrame(const RenderingData& renderingData) {
	auto& frame = inFlightFrame[currentFrame];
	auto frameIndex = currentFrame;
	currentFrame = (currentFrame + 1) % swapchain.size();
	uint32_t imageIndex;
	uint64_t timeout = 1000000000;//ns
	vkResetFences(device, 1, &frame.fenceImageAcquired);
	auto vkSwapchain = swapchain.getVkRaw();
	auto acquireResult = vkAcquireNextImageKHR(device, vkSwapchain, timeout, VK_NULL_HANDLE, frame.fenceImageAcquired, &imageIndex);
	VkFence frameFence[]{ frame.fenceImageAcquired, frame.fenceInFlight };
	//std::cout << "image acquired " << imageIndex << std::endl;
	vkWaitForFences(device, 2, frameFence, VK_TRUE, UINT64_MAX);
	//std::cout << "frame fence pass " << currentFrame << std::endl;
	//TODO find the correct timing
	retiredSwapchain.destroy();
	if (!frameResizeCheck(acquireResult, frame)) {
		return false;
	}

	releaseDescriptorPool(frame.descriptorPool);
	requestDescriptorPool(0, frame.descriptorPool);
	setupDescriptorSet(imageIndex, frame.descriptorPool);
	setupCommandBuffer(frameIndex, imageIndex);

	VkSubmitInfo submitInfo;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer[frameIndex];
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = VK_NULL_HANDLE;
	VkPipelineStageFlags waitStage[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.pWaitDstStageMask = waitStage;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &frame.semaphoreRenderFinished;

	vkResetFences(device, 1, &frame.fenceInFlight);
	//std::cout << "frame submit " << currentFrame << std::endl;
	vkQueueSubmit(graphicsQueue, 1, &submitInfo, frame.fenceInFlight);

	VkPresentInfoKHR presentInfo;
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &vkSwapchain;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &frame.semaphoreRenderFinished;
	presentInfo.pResults = nullptr;

	auto presentResult = vkQueuePresentKHR(presentQueue, &presentInfo);
	return frameResizeCheck(presentResult);
}