#pragma once
#include "glm.hpp"
#include <vector>

struct MaterialTextureEntry {
	uint16_t textureIndex;
	//TODO sampler info
	//TODO shader mapping info
};

struct MaterialValueEntry {
	glm::vec4 value;
	//TODO shader mapping info
};

class MaterialInput
{
private:
	std::vector<MaterialTextureEntry> textureEntry;
	std::vector<MaterialValueEntry> valueEntry;
	int prototypeIndex;
public:
	MaterialInput() = default;
	MaterialInput(const MaterialInput&) = delete;
	MaterialInput(MaterialInput&&) = default;
	void setPrototypeIndex(int index);
	int getPrototypeIndex() const;
	const std::vector<MaterialTextureEntry> getTextureEntry() const noexcept;
	const std::vector<MaterialValueEntry> getValueEntry() const noexcept;
	int textureCount() const;
	void addTextureEntry(const uint16_t index);
	void addValueEntry(const glm::vec4 value);
	//TODO remove an entry
};

