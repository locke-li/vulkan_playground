#include "VulkanEnv.h"
#include <set>
#include <cstdint>
#include <algorithm>
#include <iostream>
#include <fstream>

const std::vector<const char*> validationLayer = {
	"VK_LAYER_KHRONOS_validation"
};
const std::vector<const char*> extension = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
const std::string VERT_PATH = "shader/vert_simple_triangle.spv";
const std::string FRAG_PATH = "shader/frag_color.spv";

std::vector<char> loadFile(const std::string& path) {
	std::ifstream file(path, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		return std::vector<char>();
	}

	auto size = (size_t)file.tellg();
	std::vector<char> buffer(size);
	file.seekg(0);
	file.read(buffer.data(), size);
	file.close();
	return buffer;
}

bool deviceValid(const VkPhysicalDevice device) {
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceProperties(device, &properties);
	vkGetPhysicalDeviceFeatures(device, &features);

	return properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
		features.geometryShader;
}

bool deviceExtensionSupport(const VkPhysicalDevice device) {
	uint32_t count;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
	if (count == 0) return false;
	std::vector<VkExtensionProperties> properties(count);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &count, properties.data());

	int match = 0;
	for (const auto& required : extension) {
		for (const auto& ext : properties) {
			if (strcmp(required, ext.extensionName) == 0) {
				++match;
				continue;
			}
		}
	}
	return match == extension.size();
}

void querySwapChainSupport(const VkPhysicalDevice device, const VkSurfaceKHR surface, SwapChainSupport* support) {
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &support->capabilities);
	uint32_t count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr);
	if (count != 0) {
		support->formats.resize(count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, support->formats.data());
	}

	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, nullptr);
	if (count != 0) {
		support->presentMode.resize(count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, support->presentMode.data());
	}
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	for (const auto& formatInfo : availableFormats) {
		if (formatInfo.format == VK_FORMAT_R8G8B8A8_SRGB &&
			formatInfo.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return formatInfo;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& availableMode) {
	for (const auto mode : availableMode) {
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return mode;
		}
	}

	//guaranteed to be available
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
	if (capabilities.currentExtent.width == UINT32_MAX) {//flexible window?, select extent nearest to window size
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		
		return VkExtent2D{
			std::max(capabilities.minImageExtent.width, std::min(static_cast<uint32_t>(width), capabilities.maxImageExtent.width)),
			std::max(capabilities.minImageExtent.height, std::min(static_cast<uint32_t>(height), capabilities.maxImageExtent.height))
		};
	}

	return capabilities.currentExtent;
}

bool createShaderModule(const VkDevice device, const std::vector<char>& code, VkShaderModule* shaderModule) {
	VkShaderModuleCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	info.flags = 0;
	info.pNext = nullptr;
	info.pCode = reinterpret_cast<const uint32_t*>(code.data());
	info.codeSize = static_cast<uint32_t>(code.size());
	return vkCreateShaderModule(device, &info, nullptr, shaderModule) == VK_SUCCESS;
}

VkFence CreateFence(VkDevice device, VkFenceCreateFlags flags) {
	VkFence fence;
	VkFenceCreateInfo fenceInfo;
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = flags;
	fenceInfo.pNext = nullptr;
	vkCreateFence(device, &fenceInfo, nullptr, &fence);
	return fence;
}

void VulkanEnv::setWindow(GLFWwindow* win) noexcept {
	window = win;
}

void VulkanEnv::setMaxFrameInFlight(uint32_t value) noexcept {
	maxFrameInFlight = value;
}

uint32_t VulkanEnv::getWidth() const {
	return swapchainExtent.width;
}

uint32_t VulkanEnv::getHeight() const {
	return swapchainExtent.height;
}

void VulkanEnv::onFramebufferResize() noexcept {
	frameBufferResized = true;
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

	return vkCreateInstance(&info, nullptr, &instance) == VK_SUCCESS;
}

bool VulkanEnv::createSurface() {
	return glfwCreateWindowSurface(instance, window, nullptr, &surface) == VK_SUCCESS;
}

bool VulkanEnv::queueFamilyValid(const VkPhysicalDevice device) {
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
			std::cout << "execure queue family " << i << std::endl;
			result = true;
			break;
		}
		++i;
	}
	VkBool32 supportPresent = false;
	i = 0;
	for (const auto& queue : properties) {
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supportPresent);
		if (supportPresent) {
			queueFamily.present = i;
			std::cout << "present queue family " << i << std::endl;
			result &= true;
			break;
		}
		++i;
	}
	return result;
}

bool VulkanEnv::createPhysicalDevice() {
	uint32_t count;
	vkEnumeratePhysicalDevices(instance, &count, nullptr);
	if (count == 0) return false;
	std::vector<VkPhysicalDevice> deviceList(count);
	vkEnumeratePhysicalDevices(instance, &count, deviceList.data());

	for (const auto& device : deviceList) {
		querySwapChainSupport(device, surface, &swapChainSupport);
		if (deviceValid(device) &&
			queueFamilyValid(device) &&
			deviceExtensionSupport(device) &&
			!swapChainSupport.formats.empty() &&
			!swapChainSupport.presentMode.empty()) {
			physicalDevice = device;
			return true;
		}
	}

	return false;
}

bool VulkanEnv::createDevice() {
	std::set<uint32_t> uniqueQueueFamily{ queueFamily.graphics, queueFamily.present };
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

	vkGetDeviceQueue(device, queueFamily.graphics, 0, &graphicsQueue);
	vkGetDeviceQueue(device, queueFamily.present, 0, &presentQueue);

	return true;
}

bool VulkanEnv::createSwapchain() {
	frameBufferResized = false;
	swapchainFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	auto mode = choosePresentMode(swapChainSupport.presentMode);
	swapchainExtent = chooseSwapExtent(swapChainSupport.capabilities, window);
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;//one extra as buffer
	if (swapChainSupport.capabilities.maxImageCount > 0) {
		imageCount = std::min(imageCount, swapChainSupport.capabilities.maxImageCount);
	}

	VkSwapchainCreateInfoKHR info{};
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.surface = surface;
	info.minImageCount = imageCount;
	info.imageFormat = swapchainFormat.format;
	info.imageColorSpace = swapchainFormat.colorSpace;
	info.imageExtent = swapchainExtent;
	info.imageArrayLayers = 1;
	info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	info.preTransform = swapChainSupport.capabilities.currentTransform;//no pre transform
	info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;//alpha blending with other window
	info.presentMode = mode;
	info.clipped = VK_TRUE;
	info.oldSwapchain = VK_NULL_HANDLE;//used for swapchain runtime swap

	if (vkCreateSwapchainKHR(device, &info, nullptr, &swapchain) != VK_SUCCESS) {
		return false;
	}

	uint32_t count;
	vkGetSwapchainImagesKHR(device, swapchain, &count, nullptr);
	swapchainImage.resize(count);
	vkGetSwapchainImagesKHR(device, swapchain, &count, swapchainImage.data());
	return true;
}

bool VulkanEnv::createImageView() {
	swapchainImageView.resize(swapchainImage.size());
	for (auto i = 0; i < swapchainImage.size(); ++i) {
		VkImageViewCreateInfo info;
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.pNext = nullptr;
		info.flags = 0;
		info.image = swapchainImage[i];
		info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		info.format = swapchainFormat.format;
		info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		info.subresourceRange.baseMipLevel = 0;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.baseArrayLayer = 0;
		info.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device, &info, nullptr, &swapchainImageView[i]) != VK_SUCCESS) {
			return false;
		}
	}
	return true;
}

bool VulkanEnv::createRenderPass() {
	VkAttachmentDescription colorAttachment;
	colorAttachment.flags = 0;
	colorAttachment.format = swapchainFormat.format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;//don't care what it was previously
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	VkAttachmentReference colorAttachmentRef;
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass;
	subpass.flags = 0;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;//index of this maps to the "layout(location = 0) out" variable in shader code
	subpass.pDepthStencilAttachment = nullptr;
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
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	return vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) == VK_SUCCESS;
}

bool VulkanEnv::createDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding binding;
	binding.binding = 0;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	binding.descriptorCount = 1;
	binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	binding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	info.flags = 0;
	info.pNext = nullptr;
	info.bindingCount = 1;
	info.pBindings = &binding;
	if (vkCreateDescriptorSetLayout(device, &info, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		return false;
	}
	return true;
}

bool VulkanEnv::createGraphicsPipelineLayout() {
	VkPipelineLayoutCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	info.flags = 0;
	info.pNext = nullptr;
	info.setLayoutCount = 0;
	info.pSetLayouts = nullptr;
	info.pushConstantRangeCount = 0;
	info.pPushConstantRanges = nullptr;

	return vkCreatePipelineLayout(device, &info, nullptr, &graphicsPipelineLayout) == VK_SUCCESS;
}

bool VulkanEnv::createGraphicsPipeline() {
	auto vertCode = loadFile(VERT_PATH);
	auto fragCode = loadFile(FRAG_PATH);

	VkShaderModule vertShader;
	createShaderModule(device, vertCode, &vertShader);
	VkShaderModule fragShader;
	createShaderModule(device, fragCode, &fragShader);

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
	VkPipelineShaderStageCreateInfo shaderStageInfo[] = {vertStage, fragStage};

	auto vertexBinding = VertexInput::getBindingDescription();
	auto vertexAttribute = VertexInput::getAttributeDescription();

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

	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapchainExtent.width);
	viewport.height = static_cast<float>(swapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor;
	scissor.offset = { 0, 0 };
	scissor.extent = swapchainExtent;

	VkPipelineViewportStateCreateInfo viewportStateInfo;
	viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateInfo.flags = 0;
	viewportStateInfo.pNext = nullptr;
	viewportStateInfo.viewportCount = 1;
	viewportStateInfo.pViewports = &viewport;
	viewportStateInfo.scissorCount = 1;
	viewportStateInfo.pScissors = &scissor;

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
	multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
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
	pipelineInfo.pDepthStencilState = nullptr;
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

	return true;
}

bool VulkanEnv::createFrameBuffer() {
	swapchainFramebuffer.resize(swapchainImageView.size());

	for (auto i = 0; i < swapchainImageView.size(); ++i) {
		VkFramebufferCreateInfo info;
		info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info.flags = 0;
		info.pNext = nullptr;
		info.renderPass = renderPass;
		info.attachmentCount = 1;
		info.pAttachments = &swapchainImageView[i];
		info.width = swapchainExtent.width;
		info.height = swapchainExtent.height;
		info.layers = 1;

		if (vkCreateFramebuffer(device, &info, nullptr, &swapchainFramebuffer[i]) != VK_SUCCESS) {
			return false;
		}
	}

	return true;
}

bool VulkanEnv::setupBufferCopy() {
	VkFenceCreateInfo fenceInfo;
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = 0;
	fenceInfo.pNext = nullptr;
	return vkCreateFence(device, &fenceInfo, nullptr, &vertexBuffer.fenceCopy) == VK_SUCCESS &&
			vkCreateFence(device, &fenceInfo, nullptr, &indexBuffer.fenceCopy) == VK_SUCCESS;
}

bool VulkanEnv::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memTypeFlag, 
		VkBuffer& buffer, VkDeviceMemory& memory) {
	VkBufferCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.flags = 0;
	info.pNext = nullptr;
	info.usage = usage;
	info.size = size;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.queueFamilyIndexCount = 0;
	info.pQueueFamilyIndices = nullptr;

	if (vkCreateBuffer(device, &info, nullptr, &buffer) != VK_SUCCESS) {
		return false;
	}

	VkMemoryRequirements memRequirement;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirement);

	VkMemoryAllocateInfo allocInfo;
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.allocationSize = memRequirement.size;

	if (!findMemoryType(memRequirement.memoryTypeBits, memTypeFlag, &allocInfo.memoryTypeIndex)) {
		return false;
	}

	if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
		return false;
	}
	if (vkBindBufferMemory(device, buffer, memory, 0) != VK_SUCCESS) {
		return false;
	}

	return true;
}

void VulkanEnv::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size, VkFence fence, VkCommandBuffer cmd) {
	VkCommandBufferBeginInfo cmdBegin;
	cmdBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBegin.pNext = nullptr;
	cmdBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	cmdBegin.pInheritanceInfo = nullptr;
	vkBeginCommandBuffer(cmd, &cmdBegin);

	VkBufferCopy copy;
	copy.srcOffset = 0;
	copy.dstOffset = 0;
	copy.size = size;
	vkCmdCopyBuffer(cmd, src, dst, 1, &copy);

	vkEndCommandBuffer(cmd);

	VkSubmitInfo submitInfo;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = 0;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, fence);
}

bool VulkanEnv::createVertexBufferIndice(const std::vector<VertexInput*>& input) {
	uint32_t vSize = 0, iSize = 0;
	for (auto i = 0; i < input.size(); ++i) {
		const auto& vertexInput = input[i];
		vertexBuffer.offset.push_back(vSize);
		vSize += vertexInput->vertexSize();
		indexBuffer.offset.push_back(iSize);
		iSize += vertexInput->indexSize();
	}

	//TODO merge memory block alloc
	VkBuffer stagingVBuffer;
	VkDeviceMemory stagingVBufferMemory;
	createBuffer(vSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingVBuffer,
		stagingVBufferMemory);
	VkBuffer stagingIBuffer;
	VkDeviceMemory stagingIBufferMemory;
	createBuffer(vSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingIBuffer,
		stagingIBufferMemory);

	for (auto i = 0; i < input.size(); ++i) {
		const auto& vertexInput = input[i];
		void* vData;
		vkMapMemory(device, stagingVBufferMemory, vertexBuffer.offset[i], vertexInput->vertexSize(), 0, &vData);
		memcpy(vData, vertexInput->vertexData(), vertexInput->vertexSize());
		vkUnmapMemory(device, stagingVBufferMemory);
		void* iData;
		vkMapMemory(device, stagingIBufferMemory, indexBuffer.offset[i], vertexInput->indexSize(), 0, &iData);
		memcpy(iData, vertexInput->indexData(), vertexInput->indexSize());
		vkUnmapMemory(device, stagingIBufferMemory);
	}

	VkBuffer vBuffer;
	VkDeviceMemory vMemory;
	createBuffer(vSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		vBuffer,
		vMemory);
	VkBuffer iBuffer;
	VkDeviceMemory iMemory;
	createBuffer(iSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		iBuffer,
		iMemory);

	VkCommandBuffer copyCmd[2];
	VkCommandBufferAllocateInfo cmdInfo;
	cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdInfo.pNext = nullptr;
	cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdInfo.commandPool = commandPool;
	cmdInfo.commandBufferCount = 2;
	vkAllocateCommandBuffers(device, &cmdInfo, copyCmd);

	copyBuffer(stagingVBuffer, vBuffer, vSize, vertexBuffer.fenceCopy, copyCmd[0]);
	copyBuffer(stagingIBuffer, iBuffer, iSize, indexBuffer.fenceCopy, copyCmd[1]);
	VkFence fence[] = { vertexBuffer.fenceCopy, indexBuffer.fenceCopy };
	vkWaitForFences(device, 2, fence, VK_TRUE, UINT64_MAX);

	vkFreeCommandBuffers(device, commandPool, 2, copyCmd);
	vkDestroyBuffer(device, stagingVBuffer, nullptr);
	vkFreeMemory(device, stagingVBufferMemory, nullptr);
	vkDestroyBuffer(device, stagingIBuffer, nullptr);
	vkFreeMemory(device, stagingIBufferMemory, nullptr);

	vertexBuffer.input = input;
	vertexBuffer.buffer.push_back(vBuffer);
	vertexBuffer.memory.push_back(vMemory);
	indexBuffer.input = input;
	indexBuffer.buffer = iBuffer;
	indexBuffer.memory = iMemory;
	return true;
}

bool VulkanEnv::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags flags, uint32_t* typeIndex) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
		if ((typeFilter & (1 << i)) &&
			(memProperties.memoryTypes[i].propertyFlags & flags)) {
			*typeIndex = i;
			return true;
		}
	}
	return false;
}

bool VulkanEnv::createUniformBuffer() {
	VkDeviceSize size = sizeof(UniformBufferData);
	uniformBuffer.resize(swapchainImage.size());
	uniformBufferMemory.resize(swapchainImage.size());

	for (auto i = 0; i < swapchainImage.size(); ++i) {
		if (!createBuffer(size,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			uniformBuffer[i],
			uniformBufferMemory[i])) {
			return false;
		}
	}

	return true;
}

bool VulkanEnv::createCommandPool() {
	VkCommandPoolCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.flags = 0;
	info.pNext = nullptr;
	info.queueFamilyIndex = queueFamily.graphics;

	return vkCreateCommandPool(device, &info, nullptr, &commandPool) == VK_SUCCESS;
}

bool VulkanEnv::allocateCommandBuffer() {
	commandBuffer.resize(swapchainFramebuffer.size());

	VkCommandBufferAllocateInfo info;
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.pNext = nullptr;
	info.commandPool = commandPool;
	info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	info.commandBufferCount = static_cast<uint32_t>(commandBuffer.size());

	return vkAllocateCommandBuffers(device, &info, commandBuffer.data()) == VK_SUCCESS;
}

bool VulkanEnv::setupCommandBuffer() {
	for (auto i = 0; i < commandBuffer.size(); ++i) {
		VkCommandBufferBeginInfo beginInfo;
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pNext = nullptr;
		beginInfo.pInheritanceInfo = nullptr;

		auto& cmd = commandBuffer[i];
		if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) {
			return false;
		}

		VkRenderPassBeginInfo renderPassBegin;
		renderPassBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBegin.pNext = nullptr;
		renderPassBegin.framebuffer = swapchainFramebuffer[i];
		renderPassBegin.renderPass = renderPass;
		renderPassBegin.renderArea.offset = { 0, 0 };
		renderPassBegin.renderArea.extent = swapchainExtent;
		renderPassBegin.clearValueCount = 1;
		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassBegin.pClearValues = &clearColor;

		vkCmdBeginRenderPass(cmd, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
		vkCmdSetViewport(cmd, 0, 1, &viewport);
		vkCmdBindVertexBuffers(cmd, 0, static_cast<uint32_t>(vertexBuffer.buffer.size()), vertexBuffer.buffer.data(), vertexBuffer.offset.data());
		for (auto i = 0; i < indexBuffer.offset.size(); ++i) {
			vkCmdBindIndexBuffer(cmd, indexBuffer.buffer, indexBuffer.offset[i], VK_INDEX_TYPE_UINT16);
			vkCmdDrawIndexed(cmd, indexBuffer.input[i]->indexCount(), 1, 0, vertexBuffer.offset[i], 0);
		}
		vkCmdEndRenderPass(cmd);

		if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
			return false;
		}
	}

	return true;
}

bool VulkanEnv::createFrameSyncObject() {
	inFlightFrame.resize(maxFrameInFlight);

	VkSemaphoreCreateInfo semaphoreInfo;
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.flags = 0;
	semaphoreInfo.pNext = nullptr;

	VkFenceCreateInfo fenceInfo;
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	fenceInfo.pNext = nullptr;

	for (auto i = 0; i < maxFrameInFlight; ++i) {
		auto& frame = inFlightFrame[i];
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frame.semaphoreImageAquired) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frame.semaphoreRenderFinished) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &frame.fenceInFlight) != VK_SUCCESS) {
			return false;
		}
	}

	return true;
}

void VulkanEnv::destroy() {
	for (auto& frame : inFlightFrame) {
		vkDestroySemaphore(device, frame.semaphoreImageAquired, nullptr);
		vkDestroySemaphore(device, frame.semaphoreRenderFinished, nullptr);
		vkDestroyFence(device, frame.fenceInFlight, nullptr);
	}
	destroySwapchain();
	for (auto i = 0; i < vertexBuffer.buffer.size(); ++i) {
		vkDestroyBuffer(device, vertexBuffer.buffer[i], nullptr);
		vkFreeMemory(device, vertexBuffer.memory[i], nullptr);
	}
	vkDestroyBuffer(device, indexBuffer.buffer, nullptr);
	vkFreeMemory(device, indexBuffer.memory, nullptr);
	vkDestroyFence(device, vertexBuffer.fenceCopy, nullptr);
	vkDestroyFence(device, indexBuffer.fenceCopy, nullptr);
	vkDestroyCommandPool(device, commandPool, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);
}

void VulkanEnv::destroySwapchain() {
	vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffer.size()), commandBuffer.data());
	for (const auto framebuffer : swapchainFramebuffer) {
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}
	vkDestroyPipelineLayout(device, graphicsPipelineLayout, nullptr);
	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);
	for (auto i = 0; i < swapchainImageView.size(); ++i) {
		vkDestroyImageView(device, swapchainImageView[i], nullptr);
		vkDestroyBuffer(device, uniformBuffer[i], nullptr);
		vkFreeMemory(device, uniformBufferMemory[i], nullptr);
	}
	vkDestroySwapchainKHR(device, swapchain, nullptr);
}

bool VulkanEnv::recreateSwapchain() {
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	if (width == 0 || height == 0) {//minimized and/or invisible
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	//TODO exchange swapchain on the fly with VkSwapchainCreateInfoKHR.oldSwapchain
	vkDeviceWaitIdle(device);
	destroySwapchain();

	querySwapChainSupport(physicalDevice, surface, &swapChainSupport);

	bool result =
		createSwapchain() &&
		createImageView() &&
		createRenderPass() &&
		createGraphicsPipelineLayout() &&
		createGraphicsPipeline() &&
		createFrameBuffer() &&
		createUniformBuffer() &&
		allocateCommandBuffer() &&
		setupCommandBuffer();

	return result;
}

void VulkanEnv::updateUniformBuffer(const RenderingData& renderingData, const uint32_t imageIndex) {
	//TODO WIP
}

bool VulkanEnv::drawFrame(const RenderingData& renderingData) {
	auto& frame = inFlightFrame[currentFrame];
	currentFrame = (currentFrame + 1) % maxFrameInFlight;

	uint32_t imageIndex;
	vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, frame.semaphoreImageAquired, VK_NULL_HANDLE, &imageIndex);
	vkWaitForFences(device, 1, &frame.fenceInFlight, VK_TRUE, UINT64_MAX);

	updateUniformBuffer(renderingData, imageIndex);

	VkSubmitInfo submitInfo;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer[imageIndex];
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &frame.semaphoreImageAquired;
	VkPipelineStageFlags waitStage[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.pWaitDstStageMask = waitStage;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &frame.semaphoreRenderFinished;

	vkResetFences(device, 1, &frame.fenceInFlight);

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, frame.fenceInFlight) != VK_SUCCESS) {
		return false;
	}

	VkPresentInfoKHR presentInfo;
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &frame.semaphoreRenderFinished;
	presentInfo.pResults = nullptr;

	auto result = vkQueuePresentKHR(presentQueue, &presentInfo);

	if (frameBufferResized || result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		recreateSwapchain();
	}
	else if (result != VK_SUCCESS) {
		return false;
	}

	return true;
}