#pragma once
#include "MeshInput.h"

class ModelImport
{
public:
	bool load(const char* path, const float scale, MeshInput* mesh);
};

