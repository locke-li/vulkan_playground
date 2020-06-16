#include "MaterialInput.h"

void MaterialInput::setShaderIndex(int index) {
	shaderIndex = index;
}
int MaterialInput::getShaderIndex() const {
	return shaderIndex;
}

void MaterialInput::setPrototypeIndex(int index) {
	//TODO use uint32_t for indexes?
	prototypeIndex = index;
}
int MaterialInput::getPrototypeIndex() const {
	return prototypeIndex;
}

const std::vector<MaterialInput::TextureEntry> MaterialInput::getTextureEntry() const noexcept {
	return textureEntry;
}
const std::vector<MaterialInput::ValueEntry> MaterialInput::getValueEntry() const noexcept {
	return valueEntry;
}

void MaterialInput::addTextureEntry(const uint16_t index) {
	textureEntry.push_back({ index });
}

void MaterialInput::addValueEntry(const glm::vec4 value) {
	valueEntry.push_back({ value });
}

bool MaterialInput::compatibleWith(const MaterialInput::Prototype& prototype) const {
	return prototype.textureCount == textureEntry.size()
		&& prototype.valueCount == valueEntry.size();
}

MaterialInput::Prototype MaterialInput::makePrototype() const {
	return {
		static_cast<uint16_t>(textureEntry.size()),
		static_cast<uint16_t>(valueEntry.size())
	};
}