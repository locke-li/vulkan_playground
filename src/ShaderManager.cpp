#include "ShaderManager.h"

void ShaderManager::addShader(ShaderInput&& shader) {
	shaderList.push_back(shader);
}

const ShaderInput& ShaderManager::getShaderAt(const int index) const {
	return shaderList[index];
}

const size_t ShaderManager::count() const {
	return shaderList.size();
}

bool ShaderManager::preload() {
	bool ret = true;
	for (auto& shader : shaderList) {
		ret == ret && shader.preload();
	}
	return ret;
}

void ShaderManager::unload() {
	for (auto& shader : shaderList) {
		shader.unload();
	}
}