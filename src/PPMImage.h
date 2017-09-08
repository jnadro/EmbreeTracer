#pragma once

class PPMImage
{
public:
	explicit PPMImage(unsigned SizeX, unsigned SizeY);
	~PPMImage();
	PPMImage() = delete;

	void SetPixel(unsigned x, unsigned y, float r, float g, float b);
	void Write(const char * Filename) const;

	unsigned char* getPixels() { return  Pixels; }

private:
	unsigned Width = 0;
	unsigned Height = 0;
	unsigned char* Pixels = nullptr;
};
