#pragma once
#include <stdint.h>

class PPMImage
{
public:
	explicit PPMImage(uint32_t SizeX, uint32_t SizeY);
	~PPMImage();
	PPMImage() = delete;

	void GetPixel(uint32_t x, uint32_t y, float& r, float& g, float& b);
	void SetPixel(uint32_t x, uint32_t y, float r, float g, float b);
	void Write(const char * Filename, uint32_t iteration) const;

	float* getPixels() { return Pixels; }
	uint32_t getWidth() const;
	uint32_t getHeight() const;

private:
	uint32_t Width = 0;
	uint32_t Height = 0;
	float* Pixels = nullptr;
};
