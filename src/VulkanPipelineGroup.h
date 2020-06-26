#pragma once
#include "VulkanSupportStruct.h"
#include "vk_mem_alloc.h"

class VulkanSwapchain;
class ShaderInput;

class VulkanPipelineGroup {
private:
	//value copy from VulkanEnv
	VkDevice device;

	VkPipeline graphicsPipeline;
	VkViewport viewport;
public:
	void setDevice(VkDevice value);
	VkPipeline getGraphicsPipeline() const;
	const VkViewport& getViewport() const;
	bool createGraphicsPipeline(const ShaderInput& shader, VkPipelineLayout layout, VkRenderPass renderPass, const VulkanSwapchain& swapchain);
	bool createDerivedPipeline();
};