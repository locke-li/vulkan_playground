#pragma once
#include "stb_image.h"
#include <cstdint>
#include <memory>
#include <string>

class ImageInput
{
private:
	std::unique_ptr<uint8_t> pixelData;
	int byteSize;
	int width;
	int height;
	int channel;
	uint32_t mipLevel;
	bool preserve;
	bool mipmap;
	const int BytePerPixel = 4;
public:
	ImageInput(const bool preserve, const bool mipmap);
	ImageInput(const ImageInput&) = delete;
	ImageInput(ImageInput&&) = default;
	ImageInput operator=(const ImageInput&) = delete;
	ImageInput operator=(ImageInput&&) noexcept;
	~ImageInput();
	bool isValid() const;
	int getWidth() const;
	int getHeight() const;
	uint32_t getMipLevel() const;
	bool preserveData() const;
	bool shouldGenerateMipmap() const;
	uint32_t getByteSize() const;
	const uint8_t* pixel() const noexcept;
	void setMipLevel(const int offset);
	bool load(const std::string& path);
	bool loadRaw(const uint8_t* rawData, const int size);
	void release();
};

