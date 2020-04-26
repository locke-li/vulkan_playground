#pragma once
#define GLFW_INCLUDE_VULKAN 
#include "GLFW/glfw3.h";
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
	VkRenderPass renderPass;
	VkPipelineLayout graphicsPipelineLayout;
	VkPipeline graphicsPipeline;
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffer;

	int maxFrameInFlight;
	std::vector<InFlightFrame> inFlightFrame;
	uint32_t currentFrame;

	QueueFamily queueFamily;
	float queuePriority = 1.0;
	SwapChainSupport swapChainSupport;

	bool frameBufferResized;

	VkViewport viewport;
private:
	bool queueFamilyValid(const VkPhysicalDevice device);
	void destroySwapchain();
public:
	void setWindow(GLFWwindow *window) noexcept; 
	void setMaxFrameInFlight(uint32_t value) noexcept;
	void onFramebufferResize() noexcept;
	void waitUntilIdle();

	bool createInstance(const char* appName);
	bool createPhysicalDevice();
	bool createDevice();
	bool createSurface();
	bool createSwapchain();
	bool createImageView();
	bool createRenderPass();
	bool createGraphicsPipelineLayout();
	bool createGraphicsPipeline();
	bool createFrameBuffer();
	bool createCommandPool();
	bool allocateCommandBuffer();
	bool setupCommandBuffer();
	bool createFrameSyncObject();
	void destroy();

	bool recreateSwapchain();
	bool drawFrame();
};

