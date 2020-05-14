#include "WindowLayer.h"

WindowLayer::~WindowLayer() {
	destroyWindow();
}

GLFWwindow* WindowLayer::getWindow() const {
	return window;
}

bool WindowLayer::init() const {
	return glfwInit() == GLFW_TRUE;
}

bool WindowLayer::createWindow(const char* title, const int width, const int height) {
	if (window != nullptr) {
		return false;
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);//TODO wrap?
	window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	return true;
}

void WindowLayer::setEventCallback(void* ptr, GLFWframebuffersizefun resizeFun) const {
	glfwSetWindowUserPointer(window, ptr);
	glfwSetFramebufferSizeCallback(window, resizeFun);
}

bool WindowLayer::shouldClose() const {
	//TODO error handling
	auto result = glfwWindowShouldClose(window);
	return result == GLFW_TRUE;
}

void WindowLayer::handleEvent() const {
	glfwPollEvents();
}

void WindowLayer::destroyWindow() {
	if (window != nullptr) {
		glfwDestroyWindow(window);
	}
}

void WindowLayer::destroy() {
	destroyWindow();
	glfwTerminate();
}