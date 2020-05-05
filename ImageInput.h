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
	uint32_t mipLevel;
	bool perserve;
	bool mipmap;
	const int BytePerPixel = 4;
public:
	ImageInput(const bool perserve, const bool mipmap);
	bool isValid() const;
	int getWidth() const;
	int getHeight() const;
	uint32_t getMipLevel() const;
	bool perserveData() const;
	bool generateMipmap() const;
	uint32_t calculateSize() const;
	const stbi_uc* pixel() const noexcept;
	void setMipLevel(const int offset);
	bool load(const char* path);
	void release();
};

