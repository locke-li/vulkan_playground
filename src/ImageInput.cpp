#include "ImageInput.h"
#include <cmath>
#include <algorithm>
#include <iostream>

ImageInput ImageInput::operator=(ImageInput&& other) noexcept
{
	return ImageInput(std::move(other));
}

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
	return mipLevel > 1 ? mipLevel : 1;
}

bool ImageInput::preserveData() const {
	return preserve;
}

bool ImageInput::shouldGenerateMipmap() const {
	return mipLevel > 1;
}

uint32_t ImageInput::getByteSize() const {
	return byteSize;
}

const uint8_t* ImageInput::pixel() const noexcept {
	return pixelData.get();
}

void ImageInput::setPreserved(const bool value) {
	preserve = value;
}

void ImageInput::setMipLevel(const int offset) {
	mipLevel = static_cast<uint32_t>(std::floor(std::max(1.0, std::log2(std::max(width, height)) - offset)));
}

bool ImageInput::load(const std::string& path) {
	auto* data = stbi_load(path.c_str(), &width, &height, &channel, STBI_rgb_alpha);
	if (data == nullptr) {
		std::cout << stbi_failure_reason() << std::endl;
		return false;
	}
	pixelData = std::unique_ptr<uint8_t>{ data };
	byteSize = width * height * BytePerPixel;
	return true;
}

bool ImageInput::loadRaw(const uint8_t* rawData, const int size) {
	auto* data = stbi_load_from_memory(rawData, size, &width, &height, &channel, STBI_rgb_alpha);
	if (data == nullptr) {
		std::cout << stbi_failure_reason() << std::endl;
		return false;
	}
	pixelData = std::unique_ptr<uint8_t>{ data };
	byteSize = width * height * BytePerPixel;
	return true;
}

void ImageInput::release() {
	pixelData = nullptr;
}