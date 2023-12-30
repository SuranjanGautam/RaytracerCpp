#pragma once

#include "general.h"
#include "image.h"

class texture {
public:
	virtual ~texture() {};

	virtual color value(double u, double v, const point3& p)const =0;
};

class solid_color : public texture {
public:
	solid_color(color c) :color_value(c) {};
	solid_color(double r, double g, double b) : color_value(color(r, g, b)) {};

	color value(double u, double v, const point3& p) const override {
		return color_value;
	}
private:
	color color_value;
};

class checker_texture :public texture {
public:
	checker_texture(double scale, shared_ptr<texture> _even, shared_ptr<texture> _odd):inv_scale(1 / scale), even(_even), odd(_odd) {};
	checker_texture(double scale, color _even, color _odd) :inv_scale(1 / scale), even(make_shared<solid_color>(_even)), odd(make_shared<solid_color>(_odd)) {};
	
	color value(double u, double v, const point3& p) const override {
		auto xI = static_cast<int>(p.x() * inv_scale);
		auto yI = static_cast<int>(p.y() * inv_scale);
		auto zI = static_cast<int>(p.z() * inv_scale);

		bool iseven = (xI + yI + zI) % 2 == 0;

		return iseven ? even->value(u, v, p) : odd->value(u, v, p);
	}
private:
	shared_ptr<texture> odd, even;
	double inv_scale;
};


class image_texture : public texture {
public:
	image_texture(const char* filename) :_image(image(filename)) {};
	color value(double u, double v, const point3& p) const override {
		if (_image.height() <= 0) return color(0, 1, 1);

		
		u = interval(0, 1).clamp(u);
		v = 1.0 - interval(0, 1).clamp(v);

		auto i = static_cast<int>(u * _image.width());
		auto j = static_cast<int>(v * _image.height());
		auto pixel = _image.pixel_data(i, j);

		auto color_scale = 1.0 / 255.0;
		return color(color_scale * pixel[0], color_scale * pixel[1], color_scale * pixel[2]);
	}
private:
	image _image;
};