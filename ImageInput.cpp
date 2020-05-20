#include "ImageInput.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <cmath>
#include <algorithm>

ImageInput::ImageInput(const bool perserve, const bool mipmap)
	: perserve(perserve)
	, mipmap(mipmap)
	, mipLevel(0)
	, channel(0)
	, width(0)
	, height(0)
	, pixelData(nullptr)
	, byteSize(-1)
{}

ImageInput::~ImageInput() {
	release();
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
	return mipmap ? mipLevel : 1;
}

bool ImageInput::perserveData() const {
	return perserve;
}

bool ImageInput::shouldGenerateMipmap() const {
	return mipmap;
}

uint32_t ImageInput::getByteSize() const {
	return byteSize;
}

const uint8_t* ImageInput::pixel() const noexcept {
	return pixelData.get();
}

void ImageInput::setMipLevel(const int offset) {
	mipLevel = static_cast<uint32_t>(std::floor(std::max(1.0, std::log2(std::max(width, height)) - offset)));
}

bool ImageInput::load(const std::string& path) {
	pixelData = std::unique_ptr<uint8_t>{ stbi_load(path.c_str(), &width, &height, &channel, STBI_rgb_alpha) };
	byteSize = width * height * BytePerPixel;
	//TODO report error
	return pixelData != nullptr;
}

void ImageInput::setData(const uint8_t* data, const int size, const int widthIn, const int heightIn) noexcept {
	pixelData = std::unique_ptr<uint8_t>(new uint8_t[size]);
	width = widthIn;
	height = heightIn;
	//TODO channel
	byteSize = size;
	memcpy(pixelData.get(), data, size);
}

void ImageInput::release() {
	pixelData = nullptr;
}