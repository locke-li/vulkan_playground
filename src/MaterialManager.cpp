#include "MaterialManager.h"

MaterialInput& MaterialManager::getMaterial(const int index) {
	assert(index >= 0 && index < materialList.size());
	return materialList[index];
}

const MaterialInput& MaterialManager::getMaterial(const int index) const {
	assert(index >= 0 && index < materialList.size());
	return materialList[index];
}

const std::vector<MaterialInput>& MaterialManager::getMaterialList() const {
	return materialList;
}

const MaterialPrototype& MaterialManager::getPrototype(const int index) const {
	assert(index >= 0 && index < prototypeList.size());
	return prototypeList[index];
}

const std::vector<MaterialPrototype>& MaterialManager::getPrototypeList() const {
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
	for (auto i = 0; i < prototypeList.size(); ++i) {
		if (material.compatibleWith(prototypeList[i])) {
			material.setPrototypeIndex(i);
			return;
		}
	}
	material.setPrototypeIndex(static_cast<int>(prototypeList.size()));
	prototypeList.push_back(std::move(material.makePrototype()));
}