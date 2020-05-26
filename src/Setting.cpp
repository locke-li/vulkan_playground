#include "Setting.h"
#include <fstream>
#include <sstream>

bool Setting::loadFrom(const std::string& path) {
	std::ifstream input(path);
	if (!input.is_open()) {
		return false;
	}
	//NOTE for efficiency, go with C style per char parsing
	std::string line;
	while (std::getline(input, line)) {
		if (line.length() == 0 || strncmp(line.c_str(), "#", 1) == 0) continue;
		auto delimIndex = line.find('=', 0);
		std::string key = line.substr(0, delimIndex);
		++delimIndex;
		if (key == "model") {
			miscData.modelPath = std::move(line.substr(delimIndex));
			continue;
		}
		if (key == "texture") {
			miscData.texturePath = std::move(line.substr(delimIndex));
			continue;
		}
		if (key == "vertex_shader") {
			miscData.vertexShaderPath = std::move(line.substr(delimIndex));
			continue;
		}
		if (key == "fragment_shader") {
			miscData.fragmentShaderPath = std::move(line.substr(delimIndex));
			continue;
		}
		if (key == "enable_validation_layer") {
			std::istringstream(line.substr(delimIndex)) >> std::boolalpha >> miscData.enableValidationLayer;
			continue;
		}
		//silently ignores unrecognized
	}
	return true;
}