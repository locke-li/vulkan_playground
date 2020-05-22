#pragma once
#include <string>

class Setting
{
public:
	struct Graphics {
		int MSAASample = 1;
		int MaxFrameInFlight = 3;
	};
	struct Misc {
		std::string modelPath;
		std::string texturePath;
		std::string vertexShaderPath;
		std::string fragmentShaderPath;
	};
private:
	Graphics graphicsData;
	Misc miscData;
public:
	const Graphics& graphics = graphicsData;
	const Misc& misc = miscData;
	bool loadFrom(const std::string& path);
};

