#include "MeshManager.h"

void MeshManager::addMesh(MeshInput&& mesh) {
	meshList.push_back(std::move(mesh));
}

const std::vector<MeshInput>& MeshManager::getMeshList() const {
	return meshList;
}

MeshInput& MeshManager::getMeshAt(const int index) {
	return meshList.at(index);
}

const size_t MeshManager::count() const {
	return meshList.size();
}