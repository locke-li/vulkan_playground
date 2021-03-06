#pragma once
#include "MeshInput.h"
#include "MaterialManager.h"
#include "TextureManager.h"
#include <string>

struct ModelLoadingInfo{
	const float scale;
	MeshInput& mesh;
	TextureManager& texture;
	MaterialManager& material;
};

class ModelImport
{
public:
	struct Offset {
		const int texture;
		const int material;
	};
private:
	bool loadObj(const char* path, ModelLoadingInfo&& info, Offset&& offset) const;
	bool loadGltf(const std::string& path, const bool isBinary, ModelLoadingInfo&& info, Offset&& offset) const;
public:
	bool load(const std::string& path, ModelLoadingInfo&& info) const;
};

