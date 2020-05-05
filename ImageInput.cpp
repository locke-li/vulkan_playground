#include "ImageInput.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <cmath>
#include <algorithm>

ImageInput::ImageInput(const bool perserve, const bool mipmap)
	:perserve(perserve)
	,mipmap(mipmap)
	,mipLevel(0)
{}

bool ImageInput::isValid() const {
	return pixelData != nullptr;
}

int ImageInput::getWidth() const {
	return width;
}

int ImageInput::getHeight() const {
	return height;
}

uint32_t ImageInput::getMipLevel() const {
	return mipmap ? mipLevel : 1;
}

bool ImageInput::perserveData() const {
	return perserve;
}

bool ImageInput::generateMipmap() const {
	return mipmap;
}

uint32_t ImageInput::calculateSize() const {
	return width * height * BytePerPixel;
}

const stbi_uc* ImageInput::pixel() const noexcept {
	return pixelData;
}

void ImageInput::setMipLevel(const int offset) {
	mipLevel = static_cast<uint32_t>(std::floor(std::max(1.0, std::log2(std::max(width, height)) - offset)));
}

bool ImageInput::load(const char* path) {
	release();
	pixelData = stbi_load(path, &width, &height, &channel, STBI_rgb_alpha);
	return pixelData != nullptr;
}

void ImageInput::release() {
	if (pixelData != nullptr) {
		stbi_image_free(pixelData);
	}
}