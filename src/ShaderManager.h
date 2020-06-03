#pragma once
#include "ShaderInput.h"
#include <vector>

class ShaderManager
{
private:
	std::vector<ShaderInput> shaderList;
public:
	void addShader(ShaderInput&& shader);
	const ShaderInput& getShaderAt(const int index) const;
	const size_t count() const;
	bool preload();
	void unload();
};

