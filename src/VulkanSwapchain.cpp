#pragma once
#include "VulkanSwapchain.h"
#include "VulkanHelper.h"
#include <algorithm>
#include <iostream>
#include <cassert>

void VulkanSwapchain::setWindow(GLFWwindow* win) noexcept {
	window = win;
}

void VulkanSwapchain::setMaxFrameInFlight(const uint32_t value) noexcept {
	maxFrameInFlight = value;
}

void VulkanSwapchain::setInstance(VkInstance instanceIn) noexcept {
	instance = instanceIn;
}

void VulkanSwapchain::setDevice(VkDevice deviceIn) noexcept {
	device = deviceIn;
}

void VulkanSwapchain::setAllocator(VmaAllocator allocator) noexcept {
	vmaAllocator = allocator;
}

void VulkanSwapchain::setGraphicsPipeline(VkPipeline pipeline) noexcept {
	graphicsPipeline = pipeline;
}

void VulkanSwapchain::setGraphicsPipelineLayout(VkPipelineLayout layout) noexcept {
	graphicsPipelineLayout = layout;
}

void VulkanSwapchain::setRenderPass(VkRenderPass renderPassIn) noexcept {
	renderPass = renderPassIn;
}

void VulkanSwapchain::copyToBufferList(const std::vector<Buffer>& list) {
	bufferList.reserve(bufferList.size() + list.size());
	//TODO memcpy
	for (auto& buffer : list) {
		bufferList.push_back(buffer);
	}
}

void VulkanSwapchain::copyDescriptorPool(VkDescriptorPool pool) noexcept {
	descriptorPool.push_back(pool);
}

void VulkanSwapchain::setPreferedPresentMode(const VkPresentModeKHR mode) noexcept {
	preferedPresentMode = mode;
}

void VulkanSwapchain::setMsaaSample(const uint32_t count) noexcept {
	targetMsaaSample = count;
}

VkSampleCountFlagBits VulkanSwapchain::msaaSampleCount() const {
	return msaaSample;
}

uint32_t VulkanSwapchain::size() const {
	return static_cast<uint32_t>(image.size());
}

VkSurfaceKHR VulkanSwapchain::getSurface() {
	return surface;
}

const VkExtent2D& VulkanSwapchain::getExtent() const {
	return extent;
}

VkFormat VulkanSwapchain::getFormat() const {
	return format.format;
}

VkFormat VulkanSwapchain::depthFormat() const {
	return depthBuffer.format;
}

VkSwapchainKHR VulkanSwapchain::getVkRaw() const {
	return swapchain;
}

VkFramebuffer VulkanSwapchain::getFramebuffer(int index) {
	assert(index >= 0 && index < framebuffer.size());
	return framebuffer[index];
}

void VulkanSwapchain::onFramebufferResize() noexcept {
	framebufferResized = true;
}

void VulkanSwapchain::selectPhysicalDevice(const PhysicalDeviceCandidate& candidate) {
	physicalDevice = candidate.device;
	support = candidate.swapchainSupport;
	msaaSample = findUsableMsaaSampleCount(candidate.device, targetMsaaSample);
}

void VulkanSwapchain::querySupport() {
	querySwapChainSupport(physicalDevice, surface, &support);
}

bool VulkanSwapchain::createSurface() {
	return glfwCreateWindowSurface(instance, window, nullptr, &surface) == VK_SUCCESS;
}

bool VulkanSwapchain::createSwapchain() {
	framebufferResized = false;
	format = chooseSwapSurfaceFormat(support.formats);
	auto mode = choosePresentMode(support.presentMode, preferedPresentMode);
	extent = chooseSwapExtent(support.capabilities, window);
	uint32_t imageCount = std::max(support.capabilities.minImageCount, maxFrameInFlight);
	//TODO when do we need more swapchain image than frame in flight?
	if (support.capabilities.maxImageCount > 0) {
		imageCount = std::min(imageCount, support.capabilities.maxImageCount);
		if (imageCount < maxFrameInFlight) {
			std::cout << "requested max frame = " << maxFrameInFlight << ", ";
			maxFrameInFlight = imageCount == 1 ? imageCount + 1 : imageCount;
			std::cout << "but limited to " << maxFrameInFlight << std::endl;
		}
	}
	std::cout << "swapchain count = " << imageCount;
	std::cout << "[" << support.capabilities.minImageCount << "|" << support.capabilities.maxImageCount << "]" << std::endl;

	VkSwapchainCreateInfoKHR info{};
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.surface = surface;
	info.minImageCount = imageCount;
	info.imageFormat = format.format;
	info.imageColorSpace = format.colorSpace;
	info.imageExtent = extent;
	info.imageArrayLayers = 1;
	info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	info.preTransform = support.capabilities.currentTransform;//no pre transform
	info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;//alpha blending with other window
	info.presentMode = mode;
	info.clipped = VK_TRUE;
	info.oldSwapchain = swapchain;
	if (vkCreateSwapchainKHR(device, &info, nullptr, &swapchain) != VK_SUCCESS) {
		return false;
	}

	//get created swapchain image
	uint32_t count;
	vkGetSwapchainImagesKHR(device, swapchain, &count, nullptr);
	image.resize(count);
	vkGetSwapchainImagesKHR(device, swapchain, &count, image.data());

	//create swapchain image view
	imageView.resize(image.size());
	for (auto i = 0; i < image.size(); ++i) {
		if (!createImageView(device, image[i], format.format, VK_IMAGE_ASPECT_COLOR_BIT, 1, imageView[i])) {
			return false;
		}
	}
	return true;
}

bool VulkanSwapchain::createFramebuffer() {
	framebuffer.resize(image.size());
	for (auto i = 0; i < image.size(); ++i) {
		VkFramebufferCreateInfo info;
		info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info.flags = 0;
		info.pNext = nullptr;
		info.renderPass = renderPass;
		info.width = extent.width;
		info.height = extent.height;
		info.layers = 1;
		VkImageView attachments[3];
		attachments[0] = imageView[i];
		attachments[1] = depthBuffer.view;
		info.pAttachments = attachments;
		if (msaaSample == VK_SAMPLE_COUNT_1_BIT) {
			info.attachmentCount = 2;
		}
		else {
			attachments[2] = msaaColorBuffer.view;
			info.attachmentCount = 3;
		}
		if (vkCreateFramebuffer(device, &info, nullptr, &framebuffer[i]) != VK_SUCCESS) {
			return false;
		}
	}
	return true;
}

bool VulkanSwapchain::createDepthBuffer() {
	if (!findDepthFormat(physicalDevice, &depthBuffer.format)) {
		return false;
	}
	VkImageCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.flags = 0;
	info.pNext = nullptr;
	info.imageType = VK_IMAGE_TYPE_2D;
	info.extent.width = extent.width;
	info.extent.height = extent.height;
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
	if (!createImage(vmaAllocator, info, depthBuffer.image, depthBuffer.imageAllocation)) {
		return false;
	}
	if (!createImageView(device, depthBuffer.image, depthBuffer.format, VK_IMAGE_ASPECT_DEPTH_BIT, 1, depthBuffer.view)) {
		return false;
	}
	return true;
}

bool VulkanSwapchain::createMsaaColorBuffer() {
	if (msaaSample == VK_SAMPLE_COUNT_1_BIT) {
		return true;
	}
	VkImageCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.flags = 0;
	info.pNext = nullptr;
	info.imageType = VK_IMAGE_TYPE_2D;
	info.extent.width = extent.width;
	info.extent.height = extent.height;
	info.extent.depth = 1;
	info.mipLevels = 1;
	info.arrayLayers = 1;
	info.format = format.format;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.samples = msaaSample;
	info.queueFamilyIndexCount = 0;
	info.pQueueFamilyIndices = nullptr;

	if (!createImage(vmaAllocator, info, msaaColorBuffer.image, msaaColorBuffer.imageAllocation)) {
		return false;
	}
	if (!createImageView(device, msaaColorBuffer.image, format.format, VK_IMAGE_ASPECT_COLOR_BIT, 1, msaaColorBuffer.view)) {
		return false;
	}
	return true;
}

void VulkanSwapchain::waitForValidSize() {
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0) {//minimized and/or invisible
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}
}

void VulkanSwapchain::reset() {
	bufferList.clear();
	descriptorPool.clear();
	graphicsPipelineLayout = VK_NULL_HANDLE;
	graphicsPipeline = VK_NULL_HANDLE;
	renderPass = VK_NULL_HANDLE;
	swapchain = VK_NULL_HANDLE;
}

void VulkanSwapchain::destroy() {
	std::cout << "destroy swapchain " << swapchain << std::endl;
	if (swapchain == VK_NULL_HANDLE) {
		return;
	}
	for (const auto framebuffer : framebuffer) {
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}
	vkDestroyImageView(device, depthBuffer.view, nullptr);
	vmaDestroyImage(vmaAllocator, depthBuffer.image, depthBuffer.imageAllocation);
	vkDestroyImageView(device, msaaColorBuffer.view, nullptr);
	vmaDestroyImage(vmaAllocator, msaaColorBuffer.image, msaaColorBuffer.imageAllocation);
	for (auto i = 0; i < imageView.size(); ++i) {
		vkDestroyImageView(device, imageView[i], nullptr);
	}
	vkDestroyPipelineLayout(device, graphicsPipelineLayout, nullptr);
	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);
	for (const auto& buffer : bufferList) {
		vmaDestroyBuffer(vmaAllocator, buffer.buffer, buffer.allocation);
	}
	for (auto& pool : descriptorPool) {
		vkDestroyDescriptorPool(device, pool, nullptr);
	}
	vkDestroySwapchainKHR(device, swapchain, nullptr);
	reset();
}