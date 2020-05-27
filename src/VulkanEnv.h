#pragma once
#define GLFW_INCLUDE_VULKAN 
#include "GLFW/glfw3.h"
#include "vk_mem_alloc.h"
#include "MeshNode.h"
#include "MaterialManager.h"
#include "RenderingData.h"
#include "ImageInput.h"
#include "ShaderInput.h"
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
	VkSemaphore semaphoreImageAquired;
	VkSemaphore semaphoreRenderFinished;
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

class VulkanEnv
{
private:
	GLFWwindow* window;
	const RenderingData* renderingData;
	const ShaderInput* shader;
	std::vector<const char*> validationLayer;
	const MaterialManager* materialManager;

	VkInstance instance;
	VkSurfaceKHR surface;
	std::vector<PhysicalDeviceCandidate> physicalDeviceCandidate;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VmaAllocator vmaAllocator;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkSwapchainKHR swapchain;
	VkSurfaceFormatKHR swapchainFormat;
	VkExtent2D swapchainExtent;
	std::vector<VkImage> swapchainImage;
	std::vector<VkImageView> swapchainImageView;
	std::vector<VkFramebuffer> swapchainFramebuffer;
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
	bool findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags flags, uint32_t* typeIndex);
	bool findDepthFormat(VkFormat* format);
	bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage allocUsage, VkBuffer& buffer, VmaAllocation& allocation);
	bool createStagingBuffer(VkDeviceSize size, VkBuffer& buffer, VmaAllocation& allocation);
	bool createImage(const VkImageCreateInfo& info, VkImage& image, VmaAllocation& allocation);
	bool cmdTransitionImageLayout(VkCommandBuffer cmd, VkImage image, const ImageOption& option, VkImageLayout oldLyaout, VkImageLayout newLayout);
	void cmdCopyImage(VkCommandBuffer cmd, VkBuffer src, VkImage dst, uint32_t width, uint32_t height, uint32_t mipLevel);
	bool cmdGenerateTextureMipmap(VkCommandBuffer cmd, VkImage image, const ImageOption& option, uint32_t width, uint32_t height);
	void releaseDescriptorPool(VkDescriptorPool pool);
	bool requestDescriptorPool(int requirement, VkDescriptorPool& pool);
	bool createDescriptorPool(int requirement, VkDescriptorPool& pool);
	bool allocateCommandBuffer(const VkCommandPool pool, const uint32_t count, VkCommandBuffer* cmd);
	bool beginCommand(VkCommandBuffer& cmd, VkCommandBufferUsageFlags flag);
	bool submitCommand(VkCommandBuffer* cmd, uint32_t count, VkQueue queue, VkFence fence);
	bool setupDescriptorSet(int imageIndex, VkDescriptorPool pool);
	bool setupCommandBuffer(const uint32_t index, const uint32_t imageIndex);
	void destroySwapchain();
public:
	void setWindow(GLFWwindow *window) noexcept;
	void setRenderingData(const RenderingData& data) noexcept;
	void setMaterialManager(const MaterialManager&) noexcept;
	void setShader(const ShaderInput& input) noexcept;
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
	bool createVertexBufferIndice(const std::vector<const MeshInput*>& input);
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
	bool drawFrame(const RenderingData& renderingData);
};

