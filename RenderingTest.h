#pragma once
#include "WindowLayer.h"
#include "Setting.h"
#include "RenderingData.h"
#include "VulkanEnv.h"
#include <vector>

class RenderingTest
{
public:
	struct RenderContext {
		VulkanEnv* vulkanEnv;
		RenderingData* renderingData;
	};
private:
	WindowLayer windowLayer;
	Setting setting;
	RenderingData renderingData;
	VulkanEnv vulkanEnv;
	RenderContext renderContext;
	std::vector<MeshNode> modelList;
	void prepareModel();
public:
	int mainLoop();
};

