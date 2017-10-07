#include "PPMImage.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <assert.h>
#include <fstream>

PPMImage::PPMImage(uint32_t SizeX, uint32_t SizeY)
	: Width(SizeX), Height(SizeY)
{
	if (Width > 0 && Height > 0)
	{
		Pixels = new float[Width * Height * 3]();
	}
}

PPMImage::~PPMImage()
{
	if (Pixels)
	{
		delete[] Pixels;
		Pixels = nullptr;
	}
}

void PPMImage::SetPixel(uint32_t x, uint32_t y, float r, float g, float b)
{
	if (Pixels)
	{
		Pixels[((y * Width + x) * 3) + 0] = r;
		Pixels[((y * Width + x) * 3) + 1] = g;
		Pixels[((y * Width + x) * 3) + 2] = b;
	}
}

void PPMImage::Write(const char* Filename) const
{
	if (Pixels)
	{
		int returnCode = stbi_write_hdr(Filename, Width, Height, 3, Pixels);
		assert(returnCode != 0);
	}
}

uint32_t PPMImage::getWidth() const
{
	return Width;
}

uint32_t PPMImage::getHeight() const
{
	return Height;
}
