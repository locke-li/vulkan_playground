#include "VulkanEnv.h"
#include <unordered_set>
#include <cstdint>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <array>

const std::vector<const char*> validationLayer = {
	"VK_LAYER_KHRONOS_validation"
};
const std::vector<const char*> extension = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

bool deviceValid(const VkPhysicalDevice device) {
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceProperties(device, &properties);
	vkGetPhysicalDeviceFeatures(device, &features);

	return properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
		features.geometryShader &&
		features.samplerAnisotropy;
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

VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& availableMode, VkPresentModeKHR prefered) {
	for (const auto mode : availableMode) {
		if (mode == prefered) {
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

bool findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidate, VkImageTiling tiling, VkFormatFeatureFlags feature, VkFormat* format) {
	for (auto checkFormat : candidate) {
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, checkFormat, &properties);

		if ((tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & feature) == feature) ||
			(tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & feature) == feature)) {
			*format = checkFormat;
			return true;
		}
	}
	return false;
}

bool hasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
		format == VK_FORMAT_D24_UNORM_S8_UINT ||
		format == VK_FORMAT_D16_UNORM_S8_UINT;
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

bool createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect, uint32_t mipLevel, VkImageView& view) {
	VkImageViewCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	info.flags = 0;
	info.pNext = nullptr;
	info.image = image;
	info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	info.format = format;
	info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	info.subresourceRange.aspectMask = aspect;
	info.subresourceRange.baseMipLevel = 0;
	info.subresourceRange.levelCount = mipLevel;
	info.subresourceRange.baseArrayLayer = 0;
	info.subresourceRange.layerCount = 1;
	return vkCreateImageView(device, &info, nullptr, &view) == VK_SUCCESS;
}

VkSampleCountFlagBits findUsableMsaaSampleCount(VkPhysicalDevice device, uint32_t count) {
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(device, &properties);
	VkSampleCountFlags limit = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;
	while (count > 1 && (count & limit) == 0) {
		count /= 2;
	}
	return static_cast<VkSampleCountFlagBits>(count);
}

void VulkanEnv::setWindow(GLFWwindow* win) noexcept {
	window = win;
}

void VulkanEnv::setRenderingData(const RenderingData& data) noexcept {
	renderingData = &data;
}

void VulkanEnv::setShader(const ShaderInput& input) noexcept {
	shader = &input;
}

void VulkanEnv::setMaxFrameInFlight(const uint32_t value) noexcept {
	maxFrameInFlight = value;
}

void VulkanEnv::setPreferedPresentMode(const VkPresentModeKHR mode) noexcept {
	preferedPresentMode = mode;
}

void VulkanEnv::setUniformSize(const uint32_t size) noexcept {
	uniformSize = size;
}

void VulkanEnv::setMsaaSample(const uint32_t count) noexcept {
	targetMsaaSample = count;
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
			std::cout << "execute queue family " << i << std::endl;
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
			msaaSample = findUsableMsaaSampleCount(device, targetMsaaSample);
			return true;
		}
	}
	return false;
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
	vkGetDeviceQueue(device, queueFamily.graphics, 0, &graphicsQueue);
	vkGetDeviceQueue(device, queueFamily.present, 0, &presentQueue);
	return true;
}

bool VulkanEnv::createSwapchain() {
	frameBufferResized = false;
	swapchainFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	auto mode = choosePresentMode(swapChainSupport.presentMode, preferedPresentMode);
	swapchainExtent = chooseSwapExtent(swapChainSupport.capabilities, window);
	uint32_t imageCount = std::max(swapChainSupport.capabilities.minImageCount, maxFrameInFlight);
	//TODO when do we need more swapchain image than frame in flight?
	if (swapChainSupport.capabilities.maxImageCount > 0) {
		imageCount = std::min(imageCount, swapChainSupport.capabilities.maxImageCount);
		if (imageCount < maxFrameInFlight) {
			std::cout << "requested max frame = " << maxFrameInFlight << ", ";
			maxFrameInFlight = imageCount == 1 ? imageCount + 1 : imageCount;
			std::cout << "but limited to " << maxFrameInFlight << std::endl;
		}
	}
	std::cout << "swapchain count = " << imageCount;
	std::cout << "[" << swapChainSupport.capabilities.minImageCount << "|" << swapChainSupport.capabilities.maxImageCount << "]" << std::endl;

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

bool VulkanEnv::createSwapchainImageView() {
	swapchainImageView.resize(swapchainImage.size());
	for (auto i = 0; i < swapchainImage.size(); ++i) {
		if (!createImageView(device, swapchainImage[i], swapchainFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1, swapchainImageView[i])) {
			return false;
		}
	}
	return true;
}

bool VulkanEnv::createMsaaColorBuffer() {
	if (msaaSample == VK_SAMPLE_COUNT_1_BIT) {
		return true;
	}
	VkImageCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.flags = 0;
	info.pNext = nullptr;
	info.imageType = VK_IMAGE_TYPE_2D;
	info.extent.width = swapchainExtent.width;
	info.extent.height = swapchainExtent.height;
	info.extent.depth = 1;
	info.mipLevels = 1;
	info.arrayLayers = 1;
	info.format = swapchainFormat.format;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.samples = msaaSample;
	info.queueFamilyIndexCount = 0;
	info.pQueueFamilyIndices = nullptr;

	if (!createImage(info, msaaColorImage, msaaColorImageMemory)) {
		return false;
	}
	if (!createImageView(device, msaaColorImage, swapchainFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1, msaaColorImageView)) {
		return false;
	}
	return true;
}

bool VulkanEnv::createDepthBuffer() {
	if (!findDepthFormat(&depthBuffer.format)) {
		return false;
	}
	VkImageCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.flags = 0;
	info.pNext = nullptr;
	info.imageType = VK_IMAGE_TYPE_2D;
	info.extent.width = swapchainExtent.width;
	info.extent.height = swapchainExtent.height;
	info.extent.depth = 1;
	info.mipLevels = 1;
	info.arrayLayers = 1;
	info.format = depthBuffer.format;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.samples = msaaSample;
	info.queueFamilyIndexCount = 0;
	info.pQueueFamilyIndices = nullptr;
	if (!createImage(info, depthBuffer.image, depthBuffer.imageMemory)) {
		return false;
	}
	if (!createImageView(device, depthBuffer.image, depthBuffer.format, VK_IMAGE_ASPECT_DEPTH_BIT, 1, depthBuffer.view)) {
		return false;
	}
	return true;
}

bool VulkanEnv::findDepthFormat(VkFormat* format) {
	auto candidate = {
		VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM, VK_FORMAT_D16_UNORM_S8_UINT
	};
	return findSupportedFormat(physicalDevice, candidate, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, format);
}

bool VulkanEnv::createRenderPass() {
	VkAttachmentDescription colorAttachment;
	colorAttachment.flags = 0;
	colorAttachment.format = swapchainFormat.format;
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
	depthAttachment.format = depthBuffer.format;
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
	//to be filled later
	//renderPassInfo.attachmentCount
	//renderPassInfo.pAttachments
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (msaaSample != VK_SAMPLE_COUNT_1_BIT) {
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachmentRef.attachment = 2;
		VkAttachmentDescription colorResolve;
		colorResolve.flags = 0;
		colorResolve.format = swapchainFormat.format;
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

		VkAttachmentDescription attachments[] = { colorResolve, depthAttachment, colorAttachment };
		renderPassInfo.attachmentCount = 3;
		renderPassInfo.pAttachments = attachments;
	}
	else {
		VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment };
		renderPassInfo.attachmentCount = 2;
		renderPassInfo.pAttachments = attachments;
	}

	return vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) == VK_SUCCESS;
}

bool VulkanEnv::createDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding uniform;
	uniform.binding = 0;
	uniform.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniform.descriptorCount = 1;
	uniform.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uniform.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding sampler;
	sampler.binding = 1;
	sampler.descriptorCount = 1;
	sampler.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sampler.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	sampler.pImmutableSamplers = nullptr;

	std::array<VkDescriptorSetLayoutBinding, 2> binding = { uniform, sampler };
	VkDescriptorSetLayoutCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	info.flags = 0;
	info.pNext = nullptr;
	info.bindingCount = static_cast<uint32_t>(binding.size());
	info.pBindings = binding.data();
	if (vkCreateDescriptorSetLayout(device, &info, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		return false;
	}
	return true;
}

bool VulkanEnv::createGraphicsPipelineLayout() {
	VkPushConstantRange vertexConstant;
	vertexConstant.offset = 0;
	vertexConstant.size = MeshInput::getConstantSize();
	vertexConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkPipelineLayoutCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	info.flags = 0;
	info.pNext = nullptr;
	info.setLayoutCount = 1;
	info.pSetLayouts = &descriptorSetLayout;
	info.pushConstantRangeCount = 1;
	info.pPushConstantRanges = &vertexConstant;

	return vkCreatePipelineLayout(device, &info, nullptr, &graphicsPipelineLayout) == VK_SUCCESS;
}

bool VulkanEnv::createGraphicsPipeline() {
	auto vertCode = shader->getVertData();
	auto fragCode = shader->getFragData();

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
	VkPipelineShaderStageCreateInfo shaderStageInfo[] = { vertStage, fragStage };

	auto vertexBinding = MeshInput::getBindingDescription();
	auto vertexAttribute = MeshInput::getAttributeDescription();

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
	multisampleInfo.rasterizationSamples = msaaSample;
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
		info.width = swapchainExtent.width;
		info.height = swapchainExtent.height;
		info.layers = 1;
		if (msaaSample == VK_SAMPLE_COUNT_1_BIT) {
			VkImageView attachments[] = { swapchainImageView[i] , depthBuffer.view };
			info.attachmentCount = 2;
			info.pAttachments = attachments;
		}
		else {
			VkImageView attachments[] = { swapchainImageView[i] , depthBuffer.view, msaaColorImageView };
			info.attachmentCount = 3;
			info.pAttachments = attachments;
		}

		if (vkCreateFramebuffer(device, &info, nullptr, &swapchainFramebuffer[i]) != VK_SUCCESS) {
			return false;
		}
	}
	return true;
}

bool VulkanEnv::createImage(const VkImageCreateInfo& info, VkImage& image, VkDeviceMemory& imageMemory) {
	if (vkCreateImage(device, &info, nullptr, &image) != VK_SUCCESS) {
		return false;
	}

	VkMemoryRequirements memRequirement;
	vkGetImageMemoryRequirements(device, image, &memRequirement);
	VkMemoryAllocateInfo memInfo;
	memInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memInfo.pNext = nullptr;
	memInfo.allocationSize = memRequirement.size;
	if (!findMemoryType(memRequirement.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memInfo.memoryTypeIndex) ||
		vkAllocateMemory(device, &memInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		return false;
	}
	if (vkBindImageMemory(device, image, imageMemory, 0) != VK_SUCCESS) {
		return false;
	}
	return true;
}

bool VulkanEnv::createTextureImage(ImageInput& input) {
	if (!input.isValid()) {
		return false;
	}
	auto size = input.calculateSize();
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	if (!createStagingBuffer(size, stagingBuffer, stagingBufferMemory)) {
		return false;
	}

	void* buffer;
	vkMapMemory(device, stagingBufferMemory, 0, size, 0, &buffer);
	memcpy(buffer, input.pixel(), size);
	vkUnmapMemory(device, stagingBufferMemory);

	ImageOption option = { input.getMipLevel(), VK_FORMAT_R8G8B8A8_SRGB };
	VkImage image;
	VkDeviceMemory imageMemory;
	VkImageCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.flags = 0;
	info.pNext = nullptr;
	info.imageType = VK_IMAGE_TYPE_2D;
	info.extent.width = input.getWidth();
	info.extent.height = input.getHeight();
	info.extent.depth = 1;
	info.mipLevels = option.mipLevel;
	info.arrayLayers = 1;
	info.format = option.format;
	if (input.perserveData()) {
		info.tiling = VK_IMAGE_TILING_LINEAR;
		info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	}
	else {
		input.release();
		info.tiling = VK_IMAGE_TILING_OPTIMAL;
		info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	}
	info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	if (input.generateMipmap()) {
		info.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	info.queueFamilyIndexCount = 0;
	info.pQueueFamilyIndices = nullptr;
	createImage(info, image, imageMemory);
	imageSet.image.push_back(image);
	imageSet.option.push_back(option);
	imageSet.memory.push_back(imageMemory);

	std::array<VkCommandBuffer, 3> cmd;
	allocateCommandBuffer(commandPool, static_cast<uint32_t>(cmd.size()), cmd.data());
	transitionImageLayout(image, option, info.initialLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmd[0]);
	copyImage(stagingBuffer, image, input.getWidth(), input.getHeight(), 0, cmd[1]);
	if (input.generateMipmap()) {
		generateTextureMipmap(image, option, input.getWidth(), input.getHeight(), cmd[2]);
	}
	else {
		transitionImageLayout(image, option, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmd[2]);
	}

	submitCommand(cmd.data(), static_cast<uint32_t>(cmd.size()), graphicsQueue, fenceImageCopy);
	vkWaitForFences(device, 1, &fenceImageCopy, VK_TRUE, UINT64_MAX);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
	return true;
}

bool VulkanEnv::transitionImageLayout(VkImage image, const ImageOption& option, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer cmd) {
	VkFormatProperties properties;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, option.format, &properties);
	if ((properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) == 0) {
		return false;
	}
	if (!beginCommand(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)) {
		return false;
	}
	VkImageMemoryBarrier barrier;
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = nullptr;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = option.mipLevel;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags srcStage;
	VkPipelineStageFlags dstStage;
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else {
		//TODO transition combinations
		return false;
	}

	vkCmdPipelineBarrier(cmd, 
		srcStage,
		dstStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	return vkEndCommandBuffer(cmd) == VK_SUCCESS;
}

bool VulkanEnv::copyImage(VkBuffer src, VkImage dst, uint32_t width, uint32_t height, uint32_t mipLevel, VkCommandBuffer cmd) {
	if (!beginCommand(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)) {
		return false;
	}

	VkBufferImageCopy copy;
	copy.bufferOffset = 0;
	copy.bufferRowLength = 0;
	copy.bufferImageHeight = 0;
	copy.imageOffset = { 0, 0, 0 };
	copy.imageExtent = { width, height, 1 };
	copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy.imageSubresource.mipLevel = mipLevel;
	copy.imageSubresource.baseArrayLayer = 0;
	copy.imageSubresource.layerCount = 1;
	vkCmdCopyBufferToImage(cmd, src, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

	return vkEndCommandBuffer(cmd) == VK_SUCCESS;
}

bool VulkanEnv::generateTextureMipmap(VkImage image, const ImageOption& option, uint32_t width, uint32_t height, VkCommandBuffer cmd) {

	if (!beginCommand(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)) {
		return false;
	}
	VkImageMemoryBarrier barrier;
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = nullptr;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int mipWidth = width;
	int mipHeight = height;
	for (uint32_t i = 1; i < option.mipLevel; ++i) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		vkCmdPipelineBarrier(cmd,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		VkImageBlit blit;
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.srcSubresource.mipLevel = i - 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = {
			mipWidth > 1 ? mipWidth / 2 : 1,
			mipHeight > 1 ? mipHeight / 2 : 1,
			1
		};
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;
		blit.dstSubresource.mipLevel = i;
		vkCmdBlitImage(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		vkCmdPipelineBarrier(cmd,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}
	barrier.subresourceRange.baseMipLevel = option.mipLevel - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	vkCmdPipelineBarrier(cmd,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier);
	return vkEndCommandBuffer(cmd) == VK_SUCCESS;
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
	return vkAllocateMemory(device, &allocInfo, nullptr, &memory) == VK_SUCCESS &&
		vkBindBufferMemory(device, buffer, memory, 0) == VK_SUCCESS;
}

bool VulkanEnv::createStagingBuffer(VkDeviceSize size, VkBuffer& buffer, VkDeviceMemory& memory) {
	return createBuffer(size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		buffer,
		memory);
}

bool VulkanEnv::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size, VkCommandBuffer cmd) {
	if (!beginCommand(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)) {
		return false;
	}

	VkBufferCopy copy;
	copy.srcOffset = 0;
	copy.dstOffset = 0;
	copy.size = size;
	vkCmdCopyBuffer(cmd, src, dst, 1, &copy);

	return vkEndCommandBuffer(cmd) == VK_SUCCESS;
}

bool VulkanEnv::createVertexBufferIndice(const std::vector<const MeshInput*>& input) {
	uint32_t vCount = 0, vSize = 0, iSize = 0;
	for (auto i = 0; i < input.size(); ++i) {
		const auto& vertexInput = input[i];
		vertexBuffer.offset.push_back(vSize);
		indexBuffer.offset.push_back(iSize);
		indexBuffer.vOffset.push_back(vCount);
		vSize += vertexInput->vertexSize();
		vCount += vertexInput->vertexCount();
		iSize += vertexInput->indexSize();
	}

	//TODO merge memory block alloc
	VkBuffer stagingVBuffer;
	VkDeviceMemory stagingVBufferMemory;
	auto vBufferSuccess = createStagingBuffer(vSize, stagingVBuffer, stagingVBufferMemory);
	VkBuffer stagingIBuffer;
	VkDeviceMemory stagingIBufferMemory;
	auto iBufferSuccess = createStagingBuffer(vSize, stagingIBuffer, stagingIBufferMemory);
	if(!vBufferSuccess || !iBufferSuccess) {
		return false;
	}

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
	vBufferSuccess = createBuffer(vSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		vBuffer,
		vMemory);
	VkBuffer iBuffer;
	VkDeviceMemory iMemory;
	iBufferSuccess = createBuffer(iSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		iBuffer,
		iMemory);
	if (!vBufferSuccess || !iBufferSuccess) {
		return false;
	}

	VkCommandBuffer copyCmd[2];
	if (!allocateCommandBuffer(commandPool, 2, copyCmd)) {
		return false;
	}
	vBufferSuccess = copyBuffer(stagingVBuffer, vBuffer, vSize, copyCmd[0]);
	iBufferSuccess = copyBuffer(stagingIBuffer, iBuffer, iSize, copyCmd[1]);
	if (!vBufferSuccess || !iBufferSuccess) {
		return false;
	}

	if (!submitCommand(copyCmd, 2, graphicsQueue, fenceVertexIndexCopy)) {
		return false;
	}
	if (vkWaitForFences(device, 1, &fenceVertexIndexCopy, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
		return false;
	}

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

bool VulkanEnv::createDescriptorPool() {
	std::array<VkDescriptorPoolSize, 2> poolSize;
	poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize[0].descriptorCount = static_cast<uint32_t>(uniformBuffer.size());
	poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize[1].descriptorCount = static_cast<uint32_t>(uniformBuffer.size());

	VkDescriptorPoolCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	info.flags = 0;
	info.pNext = nullptr;
	info.poolSizeCount = static_cast<uint32_t>(poolSize.size());
	info.pPoolSizes = poolSize.data();
	info.maxSets = static_cast<uint32_t>(uniformBuffer.size());

	return vkCreateDescriptorPool(device, &info, nullptr, &descriptorPool) == VK_SUCCESS;
}

bool VulkanEnv::createDescriptorSet() {
	std::vector<VkDescriptorSetLayout> layout(uniformBuffer.size(), descriptorSetLayout);
	VkDescriptorSetAllocateInfo info;
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	info.pNext = nullptr;
	info.descriptorPool = descriptorPool;
	info.descriptorSetCount = static_cast<uint32_t>(layout.size());
	info.pSetLayouts = layout.data();

	descriptorSet.resize(static_cast<uint32_t>(layout.size()));
	if (vkAllocateDescriptorSets(device, &info, descriptorSet.data()) != VK_SUCCESS) {
		return false;
	}
	for (auto i = 0; i < uniformBuffer.size(); ++i) {
		VkDescriptorBufferInfo bufferInfo;
		bufferInfo.buffer = uniformBuffer[i];
		bufferInfo.offset = 0;
		bufferInfo.range = uniformSize;

		VkWriteDescriptorSet uniformWrite;
		uniformWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		uniformWrite.pNext = nullptr;
		uniformWrite.dstSet = descriptorSet[i];
		uniformWrite.dstBinding = 0;
		uniformWrite.dstArrayElement = 0;
		uniformWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformWrite.descriptorCount = 1;
		uniformWrite.pBufferInfo = &bufferInfo;
		uniformWrite.pImageInfo = nullptr;
		uniformWrite.pTexelBufferView = nullptr;

		VkDescriptorImageInfo imageInfo;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = imageSet.view[0];//TODO multiple texture
		imageInfo.sampler = imageSet.sampler[0];//TODO corresponding sampler

		VkWriteDescriptorSet samplerWrite;
		samplerWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		samplerWrite.pNext = nullptr;
		samplerWrite.dstSet = descriptorSet[i];
		samplerWrite.dstBinding = 1;
		samplerWrite.dstArrayElement = 0;
		samplerWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerWrite.descriptorCount = 1;
		samplerWrite.pBufferInfo = nullptr;
		samplerWrite.pImageInfo = &imageInfo;
		samplerWrite.pTexelBufferView = nullptr;

		std::array<VkWriteDescriptorSet, 2> writeArr = { uniformWrite, samplerWrite };
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeArr.size()), writeArr.data(), 0, nullptr);
	}
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
	commandBuffer.resize(maxFrameInFlight);
	return allocateCommandBuffer(commandPoolReset, static_cast<uint32_t>(commandBuffer.size()), commandBuffer.data());
}

bool VulkanEnv::beginCommand(VkCommandBuffer& cmd, VkCommandBufferUsageFlags flag) {
	VkCommandBufferBeginInfo info;
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.pNext = nullptr;
	info.flags = flag;
	info.pInheritanceInfo = nullptr;
	return vkBeginCommandBuffer(cmd, &info) == VK_SUCCESS;
}

bool VulkanEnv::submitCommand(VkCommandBuffer* cmd, uint32_t count, VkQueue queue, VkFence fence) {
	VkSubmitInfo info;
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.pNext = nullptr;
	info.commandBufferCount = count;
	info.pCommandBuffers = cmd;
	info.waitSemaphoreCount = 0;
	info.pWaitSemaphores = nullptr;
	info.pWaitDstStageMask = 0;
	info.signalSemaphoreCount = 0;
	info.pSignalSemaphores = nullptr;
	return vkQueueSubmit(queue, 1, &info, fence) == VK_SUCCESS;
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
	renderPassBegin.framebuffer = swapchainFramebuffer[imageIndex];
	renderPassBegin.renderPass = renderPass;
	renderPassBegin.renderArea.offset = { 0, 0 };
	renderPassBegin.renderArea.extent = swapchainExtent;
	if (msaaSample == VK_SAMPLE_COUNT_1_BIT) {
		std::array<VkClearValue, 2> clearColor;
		clearColor[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearColor[1].depthStencil = { 1.0f, 0 };
		renderPassBegin.clearValueCount = static_cast<uint32_t>(clearColor.size());
		renderPassBegin.pClearValues = clearColor.data();
	}
	else {
		std::array<VkClearValue, 3> clearColor;
		clearColor[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearColor[1].depthStencil = { 1.0f, 0 };
		clearColor[2].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassBegin.clearValueCount = static_cast<uint32_t>(clearColor.size());
		renderPassBegin.pClearValues = clearColor.data();
	}

	vkCmdBeginRenderPass(cmd, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 0, 1, &descriptorSet[imageIndex], 0, nullptr);
	vkCmdBindVertexBuffers(cmd, 0, static_cast<uint32_t>(vertexBuffer.buffer.size()), vertexBuffer.buffer.data(), vertexBuffer.offset.data());
	for (auto i = 0; i < indexBuffer.offset.size(); ++i) {
		vkCmdBindIndexBuffer(cmd, indexBuffer.buffer, indexBuffer.offset[i], VK_INDEX_TYPE_UINT16);
		auto& input = *indexBuffer.input[i];
		vkCmdPushConstants(cmd, graphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, MeshInput::getConstantSize(), &input.getConstantData());
		vkCmdDrawIndexed(cmd, input.indexCount(), 1, 0, indexBuffer.vOffset[i], 0);
	}
	vkCmdEndRenderPass(cmd);

	return vkEndCommandBuffer(cmd) == VK_SUCCESS;
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

	for (uint32_t i = 0; i < maxFrameInFlight; ++i) {
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
	vkDestroyFence(device, fenceVertexIndexCopy, nullptr);
	for (auto i = 0; i < imageSet.image.size(); ++i) {
		vkDestroyImageView(device, imageSet.view[i], nullptr);
		vkDestroyImage(device, imageSet.image[i], nullptr);
		vkFreeMemory(device, imageSet.memory[i], nullptr);
	}
	for (const auto& sampler : imageSet.sampler) {
		vkDestroySampler(device, sampler, nullptr);
	}
	vkDestroyFence(device, fenceImageCopy, nullptr);
	vkDestroyCommandPool(device, commandPool, nullptr);
	vkDestroyCommandPool(device, commandPoolReset, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);
}

void VulkanEnv::destroySwapchain() {
	for (const auto framebuffer : swapchainFramebuffer) {
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}
	vkDestroyPipelineLayout(device, graphicsPipelineLayout, nullptr);
	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);
	vkDestroyImageView(device, depthBuffer.view, nullptr);
	vkDestroyImage(device, depthBuffer.image, nullptr);
	vkFreeMemory(device, depthBuffer.imageMemory, nullptr);
	vkDestroyImageView(device, msaaColorImageView, nullptr);
	vkDestroyImage(device, msaaColorImage, nullptr);
	vkFreeMemory(device, msaaColorImageMemory, nullptr);
	for (auto i = 0; i < swapchainImageView.size(); ++i) {
		vkDestroyImageView(device, swapchainImageView[i], nullptr);
		vkDestroyBuffer(device, uniformBuffer[i], nullptr);
		vkFreeMemory(device, uniformBufferMemory[i], nullptr);
	}
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	vkDestroySwapchainKHR(device, swapchain, nullptr);
}

bool VulkanEnv::recreateSwapchain() {
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0) {//minimized and/or invisible
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	//TODO exchange swapchain on the fly with VkSwapchainCreateInfoKHR.oldSwapchain
	vkDeviceWaitIdle(device);
	destroySwapchain();
	querySwapChainSupport(physicalDevice, surface, &swapChainSupport);

	bool result =
		createSwapchain() &&
		createSwapchainImageView() &&
		createMsaaColorBuffer() &&
		createDepthBuffer() &&
		createRenderPass() &&
		createGraphicsPipelineLayout() &&
		createGraphicsPipeline() &&
		createFrameBuffer() &&
		createUniformBuffer() &&
		createDescriptorPool() &&
		createDescriptorSet() &&
		updateUniformBuffer();
	return result;
}

bool VulkanEnv::updateUniformBuffer() {
	for (auto i = 0; i < uniformBuffer.size(); ++i) {
		if (!updateUniformBuffer(i)) {
			return false;
		}
	}
	return true;
}

bool VulkanEnv::updateUniformBuffer(const uint32_t imageIndex) {
	const auto& uniform = renderingData->getUniform();
	void* buffer;
	vkMapMemory(device, uniformBufferMemory[imageIndex], 0, sizeof(uniform), 0, &buffer);
	memcpy(buffer, &uniform, sizeof(uniform));
	vkUnmapMemory(device, uniformBufferMemory[imageIndex]);
	return true;
}

bool VulkanEnv::drawFrame(const RenderingData& renderingData) {
	auto& frame = inFlightFrame[currentFrame];

	uint32_t imageIndex;
	vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, frame.semaphoreImageAquired, VK_NULL_HANDLE, &imageIndex);
	//std::cout << "image acquired " << imageIndex << std::endl;
	vkWaitForFences(device, 1, &frame.fenceInFlight, VK_TRUE, UINT64_MAX);
	//std::cout << "frame fence pass " << currentFrame << std::endl;
	setupCommandBuffer(currentFrame, imageIndex);

	VkSubmitInfo submitInfo;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer[currentFrame];
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &frame.semaphoreImageAquired;
	VkPipelineStageFlags waitStage[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.pWaitDstStageMask = waitStage;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &frame.semaphoreRenderFinished;

	vkResetFences(device, 1, &frame.fenceInFlight);
	//std::cout << "frame submit " << currentFrame << std::endl;
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
	currentFrame = (currentFrame + 1) % maxFrameInFlight;
	return true;
}