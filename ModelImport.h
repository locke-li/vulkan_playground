#pragma once
#include "MeshInput.h"
#include "TextureManager.h"
#include <string>

class ModelImport
{
private:
	bool loadObj(const char* path, const float scaling, MeshInput& mesh, TextureManager& texture) const;
	bool loadGltf(const std::string& path, const float scaling, const bool isBinary, MeshInput& mesh, TextureManager& texture) const;
public:
	bool load(const std::string& path, const float scale, MeshInput& mesh, TextureManager& texture) const;
};

