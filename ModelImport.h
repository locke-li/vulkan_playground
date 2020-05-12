#pragma once
#include "MeshInput.h"
#include <string>

class ModelImport
{
private:
	bool loadObj(const char* path, const float scale, MeshNode* mesh) const;
	bool loadGltf(const char* path, const float scale, const bool isBinary, MeshNode* mesh) const;
public:
	bool load(const std::string& path, const float scale, MeshNode* mesh) const;
};

