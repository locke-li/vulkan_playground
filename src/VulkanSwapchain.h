#pragma once
#include "VulkanSupportStruct.h"
#include "ShaderManager.h"
#include "vk_mem_alloc.h"

class VulkanSwapchain {
private:
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VmaAllocator vmaAllocator;
	GLFWwindow* window;
	VkSurfaceKHR surface;

	VkSwapchainKHR swapchain;
	VkSurfaceFormatKHR format;
	VkExtent2D extent;
	std::vector<VkImage> image;
	std::vector<VkImageView> imageView;
	std::vector<VkFramebuffer> framebuffer;
	DepthBuffer depthBuffer;
	MsaaColorBuffer msaaColorBuffer;

	SwapchainSupport support;
	uint32_t maxFrameInFlight;
	VkPresentModeKHR preferedPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	uint32_t targetMsaaSample = 1;
	VkSampleCountFlagBits msaaSample;

	bool framebufferResized;
public:
	void setMaxFrameInFlight(const uint32_t value) noexcept;
	void setInstance(VkInstance instance) noexcept;
	void setDevice(VkDevice device) noexcept;
	void setAllocator(VmaAllocator allocator) noexcept;
	void setWindow(GLFWwindow* win) noexcept;
	void setPreferedPresentMode(const VkPresentModeKHR mode) noexcept;
	void setMsaaSample(const uint32_t count) noexcept;
	VkSampleCountFlagBits msaaSampleCount() const;
	uint32_t size() const;
	VkSurfaceKHR getSurface();
	const VkExtent2D& getExtent() const;
	VkFormat getFormat() const;
	VkFormat depthFormat() const;
	VkSwapchainKHR getVkRaw() const;
	VkFramebuffer getFramebuffer(int index);
	void onFramebufferResize() noexcept;
	void selectPhysicalDevice(const PhysicalDeviceCandidate& candidate);
	void querySupport(VkPhysicalDevice physicalDevice);
	bool createSurface();
	bool createSwapchain();
	bool createFramebuffer(VkRenderPass renderPass);
	bool createDepthBuffer();
	bool createMsaaColorBuffer();
	void waitForValidSize();
	void destroy();

	inline bool resized() const {
		return framebufferResized;
	}
};