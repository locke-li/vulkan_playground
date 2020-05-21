#include "MaterialInput.h"

const std::vector<MaterialTextureEntry> MaterialInput::getTextureEntry() const noexcept {
	return textureEntry;
}
const std::vector<MaterialValueEntry> MaterialInput::getValueEntry() const noexcept {
	return valueEntry;
}

void MaterialInput::addTextureEntry(const uint16_t index) {
	textureEntry.push_back({ index });
}

void MaterialInput::addValueEntry(const glm::vec4 value) {
	valueEntry.push_back({ value });
}