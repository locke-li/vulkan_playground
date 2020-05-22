#pragma once
#include "MaterialInput.h"
#include <vector>

class MaterialManager
{
private:
	std::vector<MaterialInput> materialList;
public:
	void addMaterial(MaterialInput&&);
	MaterialInput& getMaterial(const int index);
	const MaterialInput& getMaterial(const int index) const;
	int count() const;
};

