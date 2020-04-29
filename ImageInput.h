#pragma once
#include "stb_image.h"
#include <cstdint>

class ImageInput
{
private:
	stbi_uc* pixelData;
	int width;
	int height;
	int channel;
	const int BytePerPixel = 4;
public:
	bool isValid() const;
	int getWidth() const;
	int getHeight() const;
	uint32_t calculateSize() const;
	const stbi_uc* pixel() const noexcept;
	bool load(const char* path);
	void release();
};

