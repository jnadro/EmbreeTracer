#pragma once
#include <stdint.h>

class PPMImage
{
public:
	explicit PPMImage(unsigned SizeX, unsigned SizeY);
	~PPMImage();
	PPMImage() = delete;

	void SetPixel(unsigned x, unsigned y, float r, float g, float b);
	void Write(const char * Filename) const;

	uint8_t* getPixels() { return Pixels; }

private:
	unsigned Width = 0;
	unsigned Height = 0;
	uint8_t* Pixels = nullptr;
};
