#pragma once
#include "WindowLayer.h"
#include "Setting.h"
#include "RenderingData.h"
#include "VulkanEnv.h"
#include "MeshManager.h"
#include "MaterialManager.h"
#include "ShaderManager.h"
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
	ShaderManager shaderManager;
	MaterialManager materialManager;
	MeshManager meshManager;
	int drawFailure = 0;
	void prepareModel(const Setting::Misc&);
	std::vector<MeshRenderData> setupRenderList();
public:
	int mainLoop();
};

