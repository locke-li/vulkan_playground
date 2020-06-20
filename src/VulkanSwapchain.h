#pragma once
#include "VulkanSupportStruct.h"
#include "vk_mem_alloc.h"

class VulkanSwapchain {
private:
	GLFWwindow* window;
	VkSurfaceKHR surface;

	//value copy from VulkanEnv
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VmaAllocator vmaAllocator;
	//related resources
	VkPipelineLayout graphicsPipelineLayout;
	VkPipeline graphicsPipeline;
	VkRenderPass renderPass;
	std::vector<Buffer> bufferList;
	std::vector<VkDescriptorPool> descriptorPool;

	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
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
	void setWindow(GLFWwindow* win) noexcept;
	void setMaxFrameInFlight(const uint32_t value) noexcept;
	void setInstance(VkInstance instance) noexcept;
	void setDevice(VkDevice device) noexcept;
	void setAllocator(VmaAllocator allocator) noexcept;
	void setGraphicsPipeline(VkPipeline pipeline) noexcept;
	void setGraphicsPipelineLayout(VkPipelineLayout layout) noexcept;
	void setRenderPass(VkRenderPass renderPassIn) noexcept;
	void copyToBufferList(const std::vector<Buffer>& list);
	void copyDescriptorPool(VkDescriptorPool pool) noexcept;
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
	void querySupport();
	bool createSurface();
	bool createSwapchain();
	bool createFramebuffer();
	bool createDepthBuffer();
	bool createMsaaColorBuffer();
	void waitForValidSize();
	void destroy();

	inline bool resized() const {
		return framebufferResized;
	}
};