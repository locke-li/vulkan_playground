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
	void setUserDataPtr(void* userData) const;
	void setEventCallback(GLFWframebuffersizefun resizeFun) const;
	void setKeyCallback(GLFWkeyfun keyFun) const;
	bool shouldClose() const;
	void handleEvent() const;
	void destroy();
};

