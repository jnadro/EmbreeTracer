#include "PPMImage.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <assert.h>
#include <fstream>

PPMImage::PPMImage(unsigned SizeX, unsigned SizeY)
	: Width(SizeX), Height(SizeY)
{
	if (Width > 0 && Height > 0)
	{
		Pixels = new unsigned char[Width * Height * 3]();
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

void PPMImage::SetPixel(unsigned x, unsigned y, float r, float g, float b)
{
	if (Pixels)
	{
		unsigned char red = (unsigned char)(255.0f * r);
		unsigned char green = (unsigned char)(255.0f * g);
		unsigned char blue = (unsigned char)(255.0f * b);

		Pixels[((y * Width + x) * 3) + 0] = red;
		Pixels[((y * Width + x) * 3) + 1] = green;
		Pixels[((y * Width + x) * 3) + 2] = blue;
	}
}

void PPMImage::Write(const char* Filename) const
{
	if (Pixels)
	{
		int returnCode = stbi_write_tga(Filename, Width, Height, 3, static_cast<void*>(Pixels));
		assert(returnCode != 0);
	}
}
