#include "MaterialManager.h"

MaterialInput& MaterialManager::getMaterial(const int index) {
	//TODO range check
	return materialList[index];
}

const MaterialInput& MaterialManager::getMaterial(const int index) const {
	//TODO range check
	return materialList[index];
}

const std::vector<MaterialInput>& MaterialManager::getMaterialList() const {
	return materialList;
}

const std::vector<MaterialManager::Prototype>& MaterialManager::getPrototypeList() const {
	return prototypeList;
}

int MaterialManager::count() const {
	return static_cast<int>(materialList.size());
}

void MaterialManager::addMaterial(MaterialInput&& material) {
	matchPrototype(material);
	materialList.push_back(std::move(material));
}

void MaterialManager::matchPrototype(MaterialInput& material) {
	//TODO
	prototypeList.push_back({ material.textureCount() });
	material.setPrototypeIndex(0);
}