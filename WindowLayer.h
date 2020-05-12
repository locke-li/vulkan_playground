#pragma once
#define GLFW_INCLUDE_VULKAN 
#include "GLFW/glfw3.h"

class WindowLayer
{
private:
	GLFWwindow* window;
	void destroyWindow();
public:
	~WindowLayer();
	GLFWwindow* getWindow() const;
	bool init() const;
	bool createWindow(const char* title, const int width, const int height);
	void setEventCallback(void* ptr, GLFWframebuffersizefun resizeFun) const;
	bool shouldClose() const;
	void handleEvent() const;
	void destroy();
};

