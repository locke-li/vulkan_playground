#pragma once
#include "MeshInput.h"

class ModelImport
{
private:
	MeshInput mesh;
public:
	bool load(const char* path);
};

