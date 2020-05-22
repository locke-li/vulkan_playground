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
	int count() const;
};

