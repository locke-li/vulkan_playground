#pragma once
#define GLFW_INCLUDE_VULKAN 
#include "GLFW/glfw3.h";
#include <vector>

struct QueueFamily {
	uint32_t execute;
	uint32_t present;
};

struct SwapChainSupport {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentMode;
};

class VulkanEnv
{
private:
	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkQueue queue;
	VkQueue presentQueue;
	VkSwapchainKHR swapchain;
	VkSurfaceFormatKHR swapchainFormat;
	VkExtent2D swapchainExtent;
	std::vector<VkImage> swapchainImage;
	std::vector<VkImageView> swapchainImageView;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;

	QueueFamily queueFamily;
	float queuePriority = 1.0;
	SwapChainSupport swapChainSupport;

	uint32_t windowWidth;
	uint32_t windowHeight;
private:
	bool queueFamilyValid(const VkPhysicalDevice device);
public:
	void setExtent(uint32_t width, uint32_t height);
	bool createInstance(const char* appName);
	bool createPhysicalDevice();
	bool createDevice();
	bool createSurface(GLFWwindow* window);
	bool createSwapchain();
	bool createImageView();
	bool createPipeline();

	void destroy();
};

