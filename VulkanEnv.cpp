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

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t windowWidth, uint32_t windowHeight) {
	if (capabilities.currentExtent.width == UINT32_MAX) {//flexible window?, select extent nearest to window size
		return VkExtent2D{
			std::max(capabilities.minImageExtent.width, std::min(windowWidth, capabilities.maxImageExtent.width)),
			std::max(capabilities.minImageExtent.height, std::min(windowHeight, capabilities.maxImageExtent.height))
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

void VulkanEnv::setExtent(uint32_t width, uint32_t height) {
	windowWidth = width;
	windowHeight = height;
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

bool VulkanEnv::createSurface(GLFWwindow* window) {
	return glfwCreateWindowSurface(instance, window, nullptr, &surface)  == VK_SUCCESS;
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
			queueFamily.execute = i;
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
	std::set<uint32_t> uniqueQueueFamily{ queueFamily.execute, queueFamily.present };
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

	vkGetDeviceQueue(device, queueFamily.execute, 0, &queue);
	vkGetDeviceQueue(device, queueFamily.present, 0, &presentQueue);

	return true;
}

bool VulkanEnv::createSwapchain() {
	swapchainFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	auto mode = choosePresentMode(swapChainSupport.presentMode);
	swapchainExtent = chooseSwapExtent(swapChainSupport.capabilities, windowWidth, windowHeight);
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

bool VulkanEnv::createPipeline() {
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
	auto shaderStageInfo = {vertStage, fragStage};

	VkPipelineVertexInputStateCreateInfo vertexInputInfo;
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.flags = 0;
	vertexInputInfo.pNext = nullptr;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.flags = 0;
	inputAssemblyInfo.pNext = nullptr;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport;
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = swapchainExtent.width;
	viewport.height = swapchainExtent.height;
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
		VK_DYNAMIC_STATE_LINE_WIDTH,
	};
	VkPipelineDynamicStateCreateInfo dynamicStateInfo;
	dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateInfo.flags = 0;
	dynamicStateInfo.pNext = nullptr;
	dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicState.size());
	dynamicStateInfo.pDynamicStates = dynamicState.data();

	VkPipelineLayoutCreateInfo pipelineLayoutInfo;
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.flags = 0;
	pipelineLayoutInfo.pNext = nullptr;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		return false;
	}

	vkDestroyShaderModule(device, vertShader, nullptr);
	vkDestroyShaderModule(device, fragShader, nullptr);

	return true;
}

void VulkanEnv::destroy() {
	for (const auto imageView : swapchainImageView) {
		vkDestroyImageView(device, imageView, nullptr);
	}
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroySwapchainKHR(device, swapchain, nullptr);
	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);
}