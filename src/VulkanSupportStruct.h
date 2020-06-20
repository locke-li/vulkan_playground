#pragma once

#ifndef VULKAN_SUPPORT_STRUCT_H
#define VULKAN_SUPPORT_STRUCT_H

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "vk_mem_alloc.h"
#include <vector>

struct QueueFamily {
	uint32_t graphics;
	uint32_t present;
};

struct SwapchainSupport {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentMode;
};

struct PhysicalDeviceCandidate {
	const VkPhysicalDevice& device;
	SwapchainSupport swapchainSupport;
	uint32_t score;
};

struct InFlightFrame {
	VkSemaphore semaphoreRenderFinished;
	VkFence fenceImageAquired;
	VkFence fenceInFlight;
	VkDescriptorPool descriptorPool;
};

struct Buffer {
	VkBuffer buffer;
	VmaAllocation allocation;
};

struct DrawInfo {
	int setIndex;
	const void* constantData;
};

struct DepthBuffer {
	VkImage image;
	VmaAllocation imageAllocation;
	VkImageView view;
	VkFormat format;
};

struct MsaaColorBuffer {
	VkImage image;
	VmaAllocation imageAllocation;
	VkImageView view;
};

struct VertexBuffer {
	std::vector<VkBuffer> buffer;
	std::vector<VmaAllocation> allocation;
	std::vector<VkDeviceSize> offset;
};

struct IndexBuffer {
	VkBuffer buffer;
	VmaAllocation allocation;
	std::vector<DrawInfo> drawInfo;
	std::vector<VkDeviceSize> offset;
	std::vector<uint32_t> vOffset;
	std::vector<uint32_t> iCount;
};

struct ImageOption {
	uint32_t mipLevel;
	VkFormat format;
};

struct ImageSet {
	std::vector<VkImage> image;
	std::vector<ImageOption> option;
	std::vector<VkImageView> view;
	std::vector<VmaAllocation> allocation;
	std::vector<VkSampler> sampler;
};

#endif