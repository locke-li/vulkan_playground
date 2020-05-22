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
public:
	MaterialInput() = default;
	MaterialInput(const MaterialInput&) = delete;
	MaterialInput(MaterialInput&&) = default;
	const std::vector<MaterialTextureEntry> getTextureEntry() const noexcept;
	const std::vector<MaterialValueEntry> getValueEntry() const noexcept;
	void addTextureEntry(const uint16_t index);
	void addValueEntry(const glm::vec4 value);
	//TODO remove an entry
};

