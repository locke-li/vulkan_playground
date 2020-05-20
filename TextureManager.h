#pragma once
#include "ImageInput.h"
#include <vector>

class TextureManager
{
private:
	std::vector<ImageInput> textureList;
public:
	size_t addTexture(ImageInput&& texture);
	const size_t count() const;
};

