#include "ImageInput.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

bool ImageInput::isValid() const {
	return pixelData != nullptr;
}

int ImageInput::getWidth() const {
	return width;
}

int ImageInput::getHeight() const {
	return height;
}

uint32_t ImageInput::calculateSize() const {
	return width * height * BytePerPixel;
}

const stbi_uc* ImageInput::pixel() const noexcept {
	return pixelData;
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