#pragma once
#include <iostream>
#include "vec3.h"
#include "interval.h"
#include <vector>

inline double linear_to_gamma(double _value)
{
	return sqrt(_value);
}

void write_color(std::ostream& out, color color_value, int samples_per_pixel)
{
	auto r = color_value.x();
	auto g = color_value.y();
	auto b = color_value.z();

	auto scale = 1.0 / samples_per_pixel;

	r *= scale;
	g *= scale;
	b *= scale;

	r = linear_to_gamma(r);
	g = linear_to_gamma(g);
	b = linear_to_gamma(b);

	static const interval intensity(0, 0.9999);
	std::cout << static_cast<int>(255.99 * intensity.clamp(r)) << ' ' << static_cast<int>(255.99 * intensity.clamp(g)) << ' ' << static_cast<int>(255.99 * intensity.clamp(b)) << '\n';
}

inline void write_color(std::ostream& out, color color_value)
{
	auto r = color_value.x();
	auto g = color_value.y();
	auto b = color_value.z();

	static const interval intensity(0, 0.9999);
	std::cout << static_cast<int>(255.99 * intensity.clamp(r)) << ' ' << static_cast<int>(255.99 * intensity.clamp(g)) << ' ' << static_cast<int>(255.99 * intensity.clamp(b)) << '\n';
}

color write_color(color color_value, int samples_per_pixel)
{
	auto r = color_value.x();
	auto g = color_value.y();
	auto b = color_value.z();

	auto scale = 1.0 / samples_per_pixel;

	r *= scale;
	g *= scale;
	b *= scale;

	r = linear_to_gamma(r);
	g = linear_to_gamma(g);
	b = linear_to_gamma(b);
	
	return color(r, g, b);
}

color write_color(color color_value)
{
	auto r = color_value.x();
	auto g = color_value.y();
	auto b = color_value.z();

	r = linear_to_gamma(r);
	g = linear_to_gamma(g);
	b = linear_to_gamma(b);

	return color(r, g, b);
}