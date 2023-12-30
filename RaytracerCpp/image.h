#pragma once


#ifdef _MSC_VER
#pragma warning (push, 0)
#endif
#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"

#include <cstdlib>
#include <iostream>

class image {

public:
	image() :data(nullptr) {};
	image(const char* filepath) {
		std::string filename = std::string(filepath);
		if (load(filename)) return;
		std::cout << "failed loading";
	}
	~image() {
		stbi_image_free(data);
	}
	bool load(const std::string filename)
	{
		auto n = byte_per_pixel;
		data = stbi_load(filename.c_str(), &img_width, &img_height, &n, byte_per_pixel);
		bytes_per_scanline = img_width * byte_per_pixel;
		return data != nullptr;
	}

	int width() const{ return data==nullptr?0: img_width; }
	int height() const{ return data == nullptr ? 0 : img_height; }

	const unsigned char* pixel_data(int x,int y) const
	{
		static unsigned char magenta[] = { 255,0,255 };
		if (data == nullptr) return magenta;

		x = clamp(x, 0, img_width);
		y = clamp(y, 0, img_height);

		return data + y * bytes_per_scanline + x * byte_per_pixel;
	}
private:
	const int byte_per_pixel = 3;
	unsigned char* data;
	int img_height, img_width;
	int bytes_per_scanline;

	static int clamp(int x, int low, int high)
	{
		return fmin(fmax(x,low), high - 1);
	}
};

#ifdef _MSC_VER
#pragma warning (pop)
#endif