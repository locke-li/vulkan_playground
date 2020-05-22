#include "TextureManager.h"

size_t TextureManager::addTexture(ImageInput&& texture) {
	auto index = textureList.size();
	textureList.push_back(std::move(texture));
	return index;
}

ImageInput& TextureManager::getTexture(const int index) {
	//TODO range check
	return textureList[index];
}

std::vector<ImageInput>& TextureManager::getTextureList() {
	return textureList;
}

int TextureManager::count() const {
	return static_cast<int>(textureList.size());
}