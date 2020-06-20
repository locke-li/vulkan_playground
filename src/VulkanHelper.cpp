#include "VulkanHelper.h"
#include "MeshNode.h"
#include <algorithm>
#include <iostream>

bool deviceValid(const VkPhysicalDevice device, uint32_t& score) {
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(device, &properties);

	if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += static_cast<uint32_t>(PhysicalDeviceScore::DiscreteGPU);
		return true;
	}
	if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
		score += static_cast<uint32_t>(PhysicalDeviceScore::IntegratedGPU);
		return true;
	}
	if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) {
		score += static_cast<uint32_t>(PhysicalDeviceScore::VirtualGPU);
		return true;
	}
	return false;
}

//this should be changed according to actual feature demands
bool deviceFeatureSupport(const VkPhysicalDevice device, uint32_t& score) {
	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(device, &features);
	//required
	if (features.samplerAnisotropy) {
		score += static_cast<uint32_t>(PhysicalDeviceScore::SamplerAnisotropy);
	}
	else {
		return false;
	}
	//optional
	if (features.geometryShader) {
		score += static_cast<uint32_t>(PhysicalDeviceScore::GeometryShader);
	}
	return true;
}

bool deviceExtensionSupport(const VkPhysicalDevice device, const std::vector<const char*>& extension, uint32_t optionalOffset, uint32_t& score) {
	uint32_t count;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
	if (count == 0) return false;
	std::vector<VkExtensionProperties> properties(count);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &count, properties.data());

	int match = 0;
	uint32_t i = 0;
	for (; i < optionalOffset; ++i) {
		const auto& required = extension[i];
		for (const auto& ext : properties) {
			if (strcmp(required, ext.extensionName) == 0) {
				++match;
				continue;
			}
		}
	}
	if (match != optionalOffset) {
		return false;
	}
	auto scoreAdd = static_cast<uint32_t>(PhysicalDeviceScore::OptionalExtensionAvailable);
	for (; i < extension.size(); ++i) {
		const auto& optional = extension[i];
		for (const auto& ext : properties) {
			if (strcmp(optional, ext.extensionName) == 0) {
				score += scoreAdd;
				continue;
			}
		}
	}
	return true;
}

bool querySwapChainSupport(const VkPhysicalDevice device, const VkSurfaceKHR surface, SwapchainSupport* support) {
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &support->capabilities);
	uint32_t count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr);
	if (count == 0) return false;
	support->formats.resize(count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, support->formats.data());
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, nullptr);
	if (count == 0) return false;
	support->presentMode.resize(count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, support->presentMode.data());
	return true;
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
	//FIFO is guaranteed to be available by vulkan spec
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

bool findDepthFormat(VkPhysicalDevice physicalDevice, VkFormat* format) {
	auto candidate = {
		VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM, VK_FORMAT_D16_UNORM_S8_UINT
	};
	return findSupportedFormat(physicalDevice, candidate, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, format);
}

bool hasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
		format == VK_FORMAT_D24_UNORM_S8_UINT ||
		format == VK_FORMAT_D16_UNORM_S8_UINT;
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

bool createGraphicsPipelineLayoutWithVertexConst(VkDevice device, VkDescriptorSetLayout* layout, uint32_t layoutCount, VkPipelineLayout* pipelineLayout) {
	VkPushConstantRange vertexConstant;
	vertexConstant.offset = 0;
	vertexConstant.size = MeshNode::getConstantSize();
	vertexConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkPipelineLayoutCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	info.flags = 0;
	info.pNext = nullptr;
	info.setLayoutCount = layoutCount;
	info.pSetLayouts = layout;
	info.pushConstantRangeCount = 1;
	info.pPushConstantRanges = &vertexConstant;

	return vkCreatePipelineLayout(device, &info, nullptr, pipelineLayout) == VK_SUCCESS;
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

bool createBuffer(VmaAllocator vmaAllocator, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage allocUsage, VkBuffer& buffer, VmaAllocation& allocation) {
	VkBufferCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.flags = 0;
	info.pNext = nullptr;
	info.usage = usage;
	info.size = size;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.queueFamilyIndexCount = 0;
	info.pQueueFamilyIndices = nullptr;

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = allocUsage;
	return vmaCreateBuffer(vmaAllocator, &info, &allocInfo, &buffer, &allocation, nullptr) == VK_SUCCESS;
}

bool createStagingBuffer(VmaAllocator vmaAllocator, VkDeviceSize size, VkBuffer& buffer, VmaAllocation& allocation) {
	return createBuffer(vmaAllocator, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, buffer, allocation);
}

bool createImage(VmaAllocator vmaAllocator, const VkImageCreateInfo& info, VkImage& image, VmaAllocation& allocation) {
	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	return vmaCreateImage(vmaAllocator, &info, &allocInfo, &image, &allocation, nullptr) == VK_SUCCESS;
}

bool beginCommand(VkCommandBuffer& cmd, VkCommandBufferUsageFlags flag) {
	VkCommandBufferBeginInfo info;
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.pNext = nullptr;
	info.flags = flag;
	info.pInheritanceInfo = nullptr;
	return vkBeginCommandBuffer(cmd, &info) == VK_SUCCESS;
}

bool submitCommand(VkCommandBuffer* cmd, uint32_t count, VkQueue queue, VkFence fence) {
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

void cmdCopyImage(VkCommandBuffer cmd, VkBuffer src, VkImage dst, uint32_t width, uint32_t height, uint32_t mipLevel) {
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
}

bool cmdGenerateTextureMipmap(VkCommandBuffer cmd, VkImage image, const ImageOption& option, uint32_t width, uint32_t height) {
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

bool cmdTransitionImageLayout(VkCommandBuffer cmd, VkPhysicalDevice physicalDevice, VkImage image, const ImageOption& option, VkImageLayout oldLayout, VkImageLayout newLayout) {
	VkFormatProperties properties;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, option.format, &properties);
	if ((properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) == 0) {
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
	if ((oldLayout == VK_IMAGE_LAYOUT_UNDEFINED || oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED) && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
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
		std::cout << "unsupported transition: " << oldLayout << "->" << newLayout << std::endl;
		return false;
	}

	vkCmdPipelineBarrier(cmd,
		srcStage,
		dstStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	return true;
}