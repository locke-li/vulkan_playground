#pragma once
#include "WindowLayer.h"
#include "Setting.h"
#include "RenderingData.h"
#include "VulkanEnv.h"
#include "MeshManager.h"
#include "MaterialManager.h"
#include "TextureManager.h"
#include <vector>
#include <string>

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
	TextureManager textureManager;
	MaterialManager materialManager;
	MeshManager meshManager;
	void prepareModel(const Setting::Misc&);
public:
	int mainLoop();
};

