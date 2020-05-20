#pragma once
#include "MeshInput.h"
#include <string>

class ModelImport
{
private:
	bool loadObj(const char* path, const float scaling, MeshInput& const mesh) const;
	bool loadGltf(const std::string& path, const float scaling, const bool isBinary, MeshInput& const mesh) const;
public:
	bool load(const std::string& path, const float scale, MeshInput& const mesh) const;
};

