#include "ShaderInput.h"
#include <fstream>

ShaderInput::ShaderInput(const char* vertexPath, const char* fragmentPath)
	: vertPath(vertexPath)
	, fragPath(fragmentPath)
{}

std::vector<char> ShaderInput::loadFile(const char* path) const {
	std::ifstream file(path, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		return std::vector<char>();
	}
	auto size = (size_t)file.tellg();
	//size should be multiples of 4
	std::vector<char> buffer(size);
	file.seekg(0);
	file.read(buffer.data(), size);
	file.close();
	return buffer;
}

std::vector<char> ShaderInput::getVertData() const {
	return *vertData;
}

std::vector<char> ShaderInput::getFragData() const {
	return *fragData;
}

void ShaderInput::preloadVert() {
	vertData = std::make_unique<std::vector<char>>(loadFile(vertPath));
}

void ShaderInput::preloadFrag() {
	fragData = std::make_unique<std::vector<char>>(loadFile(fragPath));
}

void ShaderInput::preload() {
	if (vertPath != nullptr) {
		preloadVert();
	}
	if (fragPath != nullptr) {
		preloadFrag();
	}
}