#pragma once
#define GLFW_INCLUDE_VULKAN 
#include "GLFW/glfw3.h"
#include "MeshInput.h"
#include "RenderingData.h"
#include "ImageInput.h"
#include "ShaderInput.h"
#include <vector>

struct QueueFamily {
	uint32_t graphics;
	uint32_t present;
};

struct SwapChainSupport {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentMode;
};

struct InFlightFrame {
	VkSemaphore semaphoreImageAquired;
	VkSemaphore semaphoreRenderFinished;
	VkFence fenceInFlight;
};

struct DepthBuffer {
	VkImage image;
	VkDeviceMemory imageMemory;
	VkImageView view;
	VkFormat format;
};

struct VertexBuffer {
	std::vector<VkBuffer> buffer;
	std::vector<VkDeviceMemory> memory;
	std::vector<MeshInput*> input;
	std::vector<VkDeviceSize> offset;
};

struct IndexBuffer {
	VkBuffer buffer;
	VkDeviceMemory memory;
	std::vector<MeshInput*> input;
	std::vector<VkDeviceSize> offset;
	std::vector<uint32_t> vOffset;
};

struct ImageOption {
	uint32_t mipLevel;
	VkFormat format;
};

struct ImageSet {
	std::vector<VkImage> image;
	std::vector<ImageOption> option;
	std::vector<VkImageView> view;
	std::vector<VkDeviceMemory> memory;
	std::vector<VkSampler> sampler;
};

class VulkanEnv
{
private:
	GLFWwindow* window;
	const RenderingData* renderingData;
	const ShaderInput* shader;

	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkSwapchainKHR swapchain;
	VkSurfaceFormatKHR swapchainFormat;
	VkExtent2D swapchainExtent;
	std::vector<VkImage> swapchainImage;
	std::vector<VkImageView> swapchainImageView;
	std::vector<VkFramebuffer> swapchainFramebuffer;
	std::vector<VkBuffer> uniformBuffer;
	std::vector<VkDeviceMemory> uniformBufferMemory;
	std::vector<VkDescriptorSet> descriptorSet;
	VkImage msaaColorImage;
	VkImageView msaaColorImageView;
	VkDeviceMemory msaaColorImageMemory;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
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

	uint32_t maxFrameInFlight;
	std::vector<InFlightFrame> inFlightFrame;
	uint32_t currentFrame;
	VkPresentModeKHR preferedPresentMode = VK_PRESENT_MODE_FIFO_KHR;

	QueueFamily queueFamily;
	float queuePriority = 1.0;
	SwapChainSupport swapChainSupport;
	uint32_t uniformSize;
	uint32_t targetMsaaSample = 1;
	VkSampleCountFlagBits msaaSample;

	bool frameBufferResized;
	VkFence fenceVertexIndexCopy;
	VkFence fenceImageCopy;

	VkViewport viewport;
private:
	bool queueFamilyValid(const VkPhysicalDevice device);
	bool findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags flags, uint32_t* typeIndex);
	bool findDepthFormat(VkFormat* format);
	bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memTypeFlag, VkBuffer& buffer, VkDeviceMemory& memory);
	bool createStagingBuffer(VkDeviceSize size, VkBuffer& buffer, VkDeviceMemory& memory);
	bool copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size, VkCommandBuffer cmd);
	bool createImage(const VkImageCreateInfo& info, VkImage& image, VkDeviceMemory& imageMemory);
	bool transitionImageLayout(VkImage image, const ImageOption& option, VkImageLayout oldLyaout, VkImageLayout newLayout, VkCommandBuffer cmd);
	bool copyImage(VkBuffer src, VkImage dst, uint32_t width, uint32_t height, uint32_t mipLevel, VkCommandBuffer cmd);
	bool generateTextureMipmap(VkImage image, const ImageOption& option, uint32_t width, uint32_t height, VkCommandBuffer cmd);
	bool allocateCommandBuffer(const VkCommandPool pool, const uint32_t count, VkCommandBuffer* cmd);
	bool beginCommand(VkCommandBuffer& cmd, VkCommandBufferUsageFlags flag);
	bool submitCommand(VkCommandBuffer* cmd, uint32_t count, VkQueue queue, VkFence fence);
	bool setupCommandBuffer(const uint32_t index, const uint32_t imageIndex);
	void destroySwapchain();
public:
	void setWindow(GLFWwindow *window) noexcept;
	void setRenderingData(const RenderingData& data) noexcept;
	void setShader(const ShaderInput& input) noexcept;
	void setMaxFrameInFlight(const uint32_t value) noexcept;
	void setPreferedPresentMode(const VkPresentModeKHR mode) noexcept;
	void setUniformSize(const uint32_t size) noexcept;
	void setMsaaSample(const uint32_t count) noexcept;
	uint32_t getWidth() const;
	uint32_t getHeight() const;
	void onFramebufferResize() noexcept;
	void waitUntilIdle();

	bool createInstance(const char* appName);
	bool createPhysicalDevice();
	bool createDevice();
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
	bool createTextureImage(ImageInput& input);
	bool createTextureImageView();
	bool createTextureSampler();
	bool setupFence();
	bool createVertexBufferIndice(const std::vector<MeshInput*>& input);
	bool createUniformBuffer();
	bool createDescriptorPool();
	bool createDescriptorSet();
	bool createCommandPool();
	bool allocateFrameCommandBuffer();
	bool createFrameSyncObject();
	void destroy();

	bool recreateSwapchain();
	bool updateUniformBuffer();
	bool updateUniformBuffer(const uint32_t imageIndex);
	bool drawFrame(const RenderingData& renderingData);
};

