#pragma once
#include "MaterialInput.h"
#include <vector>

class MaterialManager
{
private:
	std::vector<MaterialInput> materialList;
	std::vector<MaterialInput::Prototype> prototypeList;
public:
	MaterialInput& getMaterial(const int index);
	const MaterialInput& getMaterial(const int index) const;
	const std::vector<MaterialInput>& getMaterialList() const;
	const std::vector<MaterialInput::Prototype>& getPrototypeList() const;
	int count() const;
	void addMaterial(MaterialInput&&);
	void matchPrototype(MaterialInput&);
};

