#include "PPMImage.h"

#include <fstream>

PPMImage::PPMImage(unsigned SizeX, unsigned SizeY)
	: Width(SizeX), Height(SizeY)
{
	if (Width > 0 && Height > 0)
	{
		Pixels = new unsigned[Width * Height]();
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
		unsigned red = (unsigned)(255.0f * r);
		unsigned green = (unsigned)(255.0f * g);
		unsigned blue = (unsigned)(255.0f * b);

		Pixels[y * Width + x] = (red << 16) + (green << 8) + blue;
	}
}

void PPMImage::Write(const char* Filename) const
{
	if (Pixels)
	{
		std::ofstream file(Filename, std::ofstream::out | std::ofstream::trunc);

		file << "P3" << std::endl;
		file << Width << " " << Height << std::endl;
		file << "255" << std::endl;
		
		for (unsigned y = 0; y < Height; ++y)
		{
			for (unsigned x = 0; x < Width; ++x)
			{
				const unsigned& pixel = Pixels[y * Width + x];
				file << ((pixel & 0x00FF0000) >> 16) << " " << ((pixel & 0x0000FF00) >> 8) << " " << (pixel & 0x000000FF) << std::endl;
			}
		}

		file.close();
	}
}
