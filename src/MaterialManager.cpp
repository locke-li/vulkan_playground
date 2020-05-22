#include "MaterialManager.h"

void MaterialManager::addMaterial(MaterialInput&& material) {
	materialList.push_back(std::move(material));
}

MaterialInput& MaterialManager::getMaterial(const int index) {
	//TODO range check
	return materialList[index];
}

int MaterialManager::count() const {
	return static_cast<int>(materialList.size());
}