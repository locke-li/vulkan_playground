#include "MaterialInput.h"

void MaterialInput::setShaderIndex(int index) {
	shaderIndex = index;
}
int MaterialInput::getShaderIndex() const {
	return shaderIndex;
}

void MaterialInput::setPrototypeIndex(int index) {
	prototypeIndex = index;
}
int MaterialInput::getPrototypeIndex() const {
	return prototypeIndex;
}

const std::vector<MaterialTextureEntry> MaterialInput::getTextureEntry() const noexcept {
	return textureEntry;
}
const std::vector<MaterialValueEntry> MaterialInput::getValueEntry() const noexcept {
	return valueEntry;
}

int MaterialInput::textureCount() const {
	return static_cast<int>(textureEntry.size());
}

void MaterialInput::addTextureEntry(const uint16_t index) {
	textureEntry.push_back({ index });
}

void MaterialInput::addValueEntry(const glm::vec4 value) {
	valueEntry.push_back({ value });
}