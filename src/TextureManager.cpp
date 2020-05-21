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

const size_t TextureManager::count() const {
	return textureList.size();
}