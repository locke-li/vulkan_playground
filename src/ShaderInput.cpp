#include "ShaderInput.h"
#include <fstream>

ShaderInput::ShaderInput(const std::string& vertexPath, const std::string& fragmentPath)
	: vertPath(vertexPath)
	, fragPath(fragmentPath)
{}

bool ShaderInput::loadFile(const std::string& path, std::vector<char>& buffer) const {
	std::ifstream file(path, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		return false;
	}
	auto size = (size_t)file.tellg();
	//size should be multiples of 4
	buffer.resize(size);
	file.seekg(0);
	file.read(buffer.data(), size);
	file.close();
	return true;
}

const std::vector<char>& ShaderInput::getVertData() const {
	return vertData;
}

const std::vector<char>& ShaderInput::getFragData() const {
	return fragData;
}

bool ShaderInput::preloadVert() {
	return !vertPath.empty() && loadFile(vertPath, vertData);
}

bool ShaderInput::preloadFrag() {
	return !fragPath.empty() && loadFile(fragPath, fragData);
}

bool ShaderInput::preload() {
	return (vertPath.empty() || preloadVert()) && (fragPath.empty() || preloadFrag());
}

void ShaderInput::unload() {
	vertData.resize(0);
	fragData.resize(0);
}