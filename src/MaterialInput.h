#pragma once
#include "glm.hpp"
#include <vector>

class MaterialInput
{
private:
	struct TextureEntry {
		uint16_t textureIndex;
		//TODO sampler info
		//TODO shader mapping info
	};

	struct ValueEntry {
		glm::vec4 value;
		//TODO shader mapping info
	};
public:
	struct Prototype {
		uint16_t textureCount;
		uint16_t valueCount;
	};
private:
	std::vector<TextureEntry> textureEntry;
	std::vector<ValueEntry> valueEntry;
	int shaderIndex = 0;
	int prototypeIndex;
public:
	MaterialInput() = default;
	MaterialInput(const MaterialInput&) = delete;
	MaterialInput(MaterialInput&&) = default;
	void setShaderIndex(int index);
	int getShaderIndex() const;
	void setPrototypeIndex(int index);
	int getPrototypeIndex() const;
	const std::vector<TextureEntry> getTextureEntry() const noexcept;
	const std::vector<ValueEntry> getValueEntry() const noexcept;
	void addTextureEntry(const uint16_t index);
	void addValueEntry(const glm::vec4 value);
	//TODO remove an entry
	bool compatibleWith(const Prototype& prototype) const;
	Prototype makePrototype() const;
};

