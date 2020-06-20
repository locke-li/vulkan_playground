#pragma once
#include "VulkanSupportStruct.h"
#include <vector>

enum class PhysicalDeviceScore : uint32_t {
	//GPU type
	DiscreteGPU = 300,
	IntegratedGPU = 200,
	VirtualGPU = 100,
	//feature
	GeometryShader = 10,
	SamplerAnisotropy = 10,
	//extension
	OptionalExtensionAvailable = 20,
	//queue family
	QueueFamilyValid = 1000,
};

//device
bool deviceValid(const VkPhysicalDevice device, uint32_t& score);
bool deviceFeatureSupport(const VkPhysicalDevice device, uint32_t& score);
bool deviceExtensionSupport(const VkPhysicalDevice device, const std::vector<const char*>& extension, uint32_t optionalOffset, uint32_t& score);

//swapchain
bool querySwapChainSupport(const VkPhysicalDevice device, const VkSurfaceKHR surface, SwapchainSupport* support);
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& availableMode, VkPresentModeKHR prefered);
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);

bool findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidate, VkImageTiling tiling, VkFormatFeatureFlags feature, VkFormat* format);
bool findDepthFormat(VkPhysicalDevice physicalDevice, VkFormat* format);
bool hasStencilComponent(VkFormat format);

VkSampleCountFlagBits findUsableMsaaSampleCount(VkPhysicalDevice device, uint32_t count);
bool createShaderModule(const VkDevice device, const std::vector<char>& code, VkShaderModule* shaderModule);
bool createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect, uint32_t mipLevel, VkImageView& view);

//layout
bool createGraphicsPipelineLayoutWithVertexConst(VkDevice device, VkDescriptorSetLayout* layout, uint32_t layoutCount, VkPipelineLayout* pipelineLayout);

//memory
bool createBuffer(VmaAllocator vmaAllocator, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage allocUsage, VkBuffer& buffer, VmaAllocation& allocation);
bool createStagingBuffer(VmaAllocator vmaAllocator, VkDeviceSize size, VkBuffer& buffer, VmaAllocation& allocation);
bool createImage(VmaAllocator vmaAllocator, const VkImageCreateInfo& info, VkImage& image, VmaAllocation& allocation);

//command buffer
bool beginCommand(VkCommandBuffer& cmd, VkCommandBufferUsageFlags flag);
bool submitCommand(VkCommandBuffer* cmd, uint32_t count, VkQueue queue, VkFence fence);

//image
void cmdCopyImage(VkCommandBuffer cmd, VkBuffer src, VkImage dst, uint32_t width, uint32_t height, uint32_t mipLevel);
bool cmdGenerateTextureMipmap(VkCommandBuffer cmd, VkImage image, const ImageOption& option, uint32_t width, uint32_t height);
bool cmdTransitionImageLayout(VkCommandBuffer cmd, VkPhysicalDevice physicalDevice, VkImage image, const ImageOption& option, VkImageLayout oldLayout, VkImageLayout newLayout);