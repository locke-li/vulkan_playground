#pragma once
#include "MeshInput.h"
#include <vector>

class MeshManager
{
private:
	std::vector<MeshInput> meshList;
public:
	void addMesh(MeshInput&& mesh);
	const std::vector<MeshInput>& getMeshList() const;
	MeshInput& getMeshAt(const int index);
	const size_t count() const;
};

