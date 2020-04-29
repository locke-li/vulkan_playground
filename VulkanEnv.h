#pragma once
#define GLFW_INCLUDE_VULKAN 
#include "GLFW/glfw3.h"
#include "VertexInput.h"
#include "RenderingData.h"
#include "ImageInput.h"
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

struct VertexBuffer {
	std::vector<VkBuffer> buffer;
	std::vector<VkDeviceMemory> memory;
	std::vector<VertexInput*> input;
	std::vector<VkDeviceSize> offset;
};

struct IndexBuffer {
	VkBuffer buffer;
	VkDeviceMemory memory;
	std::vector<VertexInput*> input;
	std::vector<VkDeviceSize> offset;
};

struct ImageSet {
	std::vector<VkImage> image;
	std::vector<VkDeviceMemory> memory;
};

class VulkanEnv
{
private:
	GLFWwindow *window;
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
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	VkRenderPass renderPass;
	VkPipelineLayout graphicsPipelineLayout;
	VkPipeline graphicsPipeline;
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffer;
	VertexBuffer vertexBuffer;
	IndexBuffer indexBuffer;
	ImageSet imageSet;

	int maxFrameInFlight;
	std::vector<InFlightFrame> inFlightFrame;
	uint32_t currentFrame;

	QueueFamily queueFamily;
	float queuePriority = 1.0;
	SwapChainSupport swapChainSupport;

	bool frameBufferResized;
	VkFence fenceVertexIndexCopy;
	VkFence fenceImageCopy;

	VkViewport viewport;
private:
	bool queueFamilyValid(const VkPhysicalDevice device);
	bool findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags flags, uint32_t* typeIndex);
	bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memTypeFlag, VkBuffer& buffer, VkDeviceMemory& memory);
	bool createStagingBuffer(VkDeviceSize size, VkBuffer& buffer, VkDeviceMemory& memory);
	bool copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size, VkCommandBuffer cmd);
	bool transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLyaout, VkImageLayout newLayout, VkCommandBuffer cmd);
	bool copyImage(VkBuffer src, VkImage dst, uint32_t width, uint32_t height, VkCommandBuffer cmd);
	bool allocateCommandBuffer(uint32_t count, VkCommandBuffer* cmd);
	bool beginCommand(VkCommandBuffer& cmd, VkCommandBufferUsageFlags flag);
	bool submitCommand(VkCommandBuffer* cmd, uint32_t count, VkQueue queue, VkFence fence);
	void destroySwapchain();
public:
	void setWindow(GLFWwindow *window) noexcept; 
	void setMaxFrameInFlight(uint32_t value) noexcept;
	uint32_t getWidth() const;
	uint32_t getHeight() const;
	void onFramebufferResize() noexcept;
	void waitUntilIdle();

	bool createInstance(const char* appName);
	bool createPhysicalDevice();
	bool createDevice();
	bool createSurface();
	bool createSwapchain();
	bool createImageView();
	bool createRenderPass();
	bool createDescriptorSetLayout();
	bool createGraphicsPipelineLayout();
	bool createGraphicsPipeline();
	bool createFrameBuffer();
	bool createTextureImage(ImageInput& input, bool perserveInput);
	bool setupFence();
	bool createVertexBufferIndice(const std::vector<VertexInput*>& input);
	bool createUniformBuffer();
	bool createDescriptorPool();
	bool createDescriptorSet();
	bool createCommandPool();
	bool allocateSwapchainCommandBuffer();
	bool setupCommandBuffer();
	bool createFrameSyncObject();
	void destroy();

	bool recreateSwapchain();
	void updateUniformBuffer(const RenderingData& renderingData, const uint32_t imageIndex);
	bool drawFrame(const RenderingData& renderingData);
};

