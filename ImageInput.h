#pragma once
#include "stb_image.h"
#include <cstdint>
#include <string>

class ImageInput
{
private:
	uint8_t* pixelData;
	int width;
	int height;
	int channel;
	uint32_t mipLevel;
	bool perserve;
	bool mipmap;
	const int BytePerPixel = 4;
public:
	ImageInput(const bool perserve, const bool mipmap);
	~ImageInput();
	bool isValid() const;
	int getWidth() const;
	int getHeight() const;
	uint32_t getMipLevel() const;
	bool perserveData() const;
	bool generateMipmap() const;
	uint32_t calculateSize() const;
	const uint8_t* pixel() const noexcept;
	void setMipLevel(const int offset);
	bool load(const std::string& path);
	void setData(uint8_t* pixelData) noexcept;
	void release();
};

