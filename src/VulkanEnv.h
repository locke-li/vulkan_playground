#pragma once
#include "vk_mem_alloc.h"
#include "MeshNode.h"
#include "ShaderManager.h"
#include "MaterialManager.h"
#include "RenderingData.h"
#include "ImageInput.h"
#include "ShaderInput.h"
#include "VulkanSupportStruct.h"
#include "VulkanSwapchain.h"
#include <vector>

class VulkanEnv
{
private:
	const RenderingData* renderingData;
	std::vector<const char*> validationLayer;
	const MaterialManager* materialManager;
	ShaderManager* shaderManager;

	VkInstance instance;
	std::vector<PhysicalDeviceCandidate> physicalDeviceCandidate;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VmaAllocator vmaAllocator;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VulkanSwapchain swapchain;
	std::vector<Buffer> uniformBufferMatrix;
	std::vector<Buffer> uniformBufferLight;
	std::vector<std::vector<VkDescriptorSet>> descriptorSet;
	std::vector<VkDescriptorPool> descriptorPool;
	std::vector<VkDescriptorPool> descriptorPoolFree;
	//TODO use pipeline cache
	VkDescriptorSetLayout descriptorSetLayoutUniform;
	std::vector<VkDescriptorSetLayout> descriptorSetLayoutMaterial;
	VkRenderPass renderPass;
	VkPipelineLayout graphicsPipelineLayout;
	VkPipeline graphicsPipeline;
	VkCommandPool commandPool;
	VkCommandPool commandPoolReset;
	std::vector<VkCommandBuffer> commandBuffer;
	VertexBuffer vertexBuffer;
	IndexBuffer indexBuffer;
	ImageSet imageSet;
	DepthBuffer depthBuffer;
	MsaaColorBuffer msaaColorBuffer;

	uint32_t maxFrameInFlight;
	std::vector<const char*> extension;
	uint32_t optionalExtensionOffset;
	std::vector<InFlightFrame> inFlightFrame;
	uint32_t currentFrame;
	VkPresentModeKHR preferedPresentMode = VK_PRESENT_MODE_FIFO_KHR;

	QueueFamily queueFamily;
	float queuePriority = 1.0;
	SwapchainSupport swapchainSupport;
	uint32_t targetMsaaSample = 1;
	VkSampleCountFlagBits msaaSample;

	bool frameBufferResized;
	VkFence fenceVertexIndexCopy;
	VkFence fenceImageCopy;

	VkViewport viewport;
private:
	bool queueFamilyValid(const VkPhysicalDevice device, uint32_t& score);
	void releaseDescriptorPool(VkDescriptorPool pool);
	bool requestDescriptorPool(int requirement, VkDescriptorPool& pool);
	bool createDescriptorPool(int requirement, VkDescriptorPool& pool);
	bool allocateCommandBuffer(const VkCommandPool pool, const uint32_t count, VkCommandBuffer* cmd);
	bool setupDescriptorSet(int imageIndex, VkDescriptorPool pool);
	bool setupCommandBuffer(const uint32_t index, const uint32_t imageIndex);
	void destroySwapchain();
public:
	void setWindow(GLFWwindow *window) noexcept;
	void setRenderingData(const RenderingData& data) noexcept;
	void setRenderingManager(const MaterialManager&, ShaderManager&) noexcept;
	void setMaxFrameInFlight(const uint32_t value) noexcept;
	void setPreferedPresentMode(const VkPresentModeKHR mode) noexcept;
	void setMsaaSample(const uint32_t count) noexcept;
	uint32_t getWidth() const;
	uint32_t getHeight() const;
	void enableValidationLayer(std::vector<const char*>&& layer);
	void selectPhysicalDevice(const PhysicalDeviceCandidate& candidate);
	void onFramebufferResize() noexcept;
	void waitUntilIdle();

	bool createInstance(const char* appName);
	bool createPhysicalDevice();
	bool createDevice();
	bool createAllocator();
	bool createSurface();
	bool createSwapchain();
	bool createSwapchainImageView();
	bool createMsaaColorBuffer();
	bool createDepthBuffer();
	bool createRenderPass();
	bool createDescriptorSetLayout();
	bool createGraphicsPipelineLayout();
	bool createGraphicsPipeline();
	bool createFrameBuffer();
	bool createTextureImage(const std::vector<ImageInput>& input);
	bool createTextureImageView();
	bool createTextureSampler();
	bool setupFence();
	bool createVertexBufferIndice();
	bool createUniformBuffer();
	bool prepareDescriptor();
	bool createCommandPool();
	bool allocateFrameCommandBuffer();
	bool createFrameSyncObject();
	void destroy();

	bool recreateSwapchain();
	bool updateUniformBuffer();
	bool updateUniformBufferMatrix(const uint32_t imageIndex);
	bool updateUniformBufferLight(const uint32_t imageIndex);
	bool frameDrawCheck(VkResult result);
	bool drawFrame(const RenderingData& renderingData);
};

