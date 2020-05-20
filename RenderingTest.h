#pragma once
#include "WindowLayer.h"
#include "Setting.h"
#include "RenderingData.h"
#include "VulkanEnv.h"
#include "MeshInput.h"
#include <vector>
#include <string>

struct TestInput {
	std::string modelPath;
	std::string texturePath;
	std::string vertexShaderPath;
	std::string fragmentShaderPath;
};

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
	std::vector<MeshInput> modelList;
	void prepareModel(const TestInput&);
	bool readInput(TestInput& inputOut) const;
public:
	int mainLoop();
};

