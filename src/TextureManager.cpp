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

void TextureManager::releaseNonPreserved() {
	for (auto& tex : textureList) {
		if (!tex.preserveData()) {
			tex.release();
		}
	}
}

void TextureManager::cleanup() {
	auto iter = std::remove_if(textureList.begin(), textureList.end(),
		[](const ImageInput& tex) {
			return !tex.preserveData();
		}
	);
	textureList.erase(iter, textureList.end());
}