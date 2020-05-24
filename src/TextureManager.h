#pragma once
#include "ImageInput.h"
#include <vector>

class TextureManager
{
private:
	std::vector<ImageInput> textureList;
public:
	size_t addTexture(ImageInput&& texture);
	ImageInput& getTexture(const int index);
	std::vector<ImageInput>& getTextureList();
	int count() const;
	void releaseNonPreserved();
};

