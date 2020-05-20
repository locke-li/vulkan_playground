#include "TextureManager.h"

size_t TextureManager::addTexture(ImageInput&& texture) {
	auto index = textureList.size();
	textureList.push_back(std::move(texture));
	return index;
}

const size_t TextureManager::count() const {
	return textureList.size();
}