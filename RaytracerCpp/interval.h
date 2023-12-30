#pragma once

#include "general.h"
class interval {
public:
	double min, max;


	interval():min(infinity),max(-infinity) {}; //empty
	interval(double _min, double _max) :min(_min), max(_max) {};
	interval(const interval& a, const interval& b) :min(fmin(a.min, b.min)), max(fmax(a.max, b.max)) {};
	

	bool contains(double value) const {
		return value >= min && value <= max;
	}

	bool sorrounds(double value) const {
		return value > min && value < max;
	}

	double clamp(double value) const {
		return std::min(std::max(value, min), max);
	}

	interval expand(double delta) const {
		auto padding = delta / 2;
		return interval(min - padding, max + padding);
	}

	double size() const {
		return fabs(max - min);
	}

	static const interval empty, universe;
};

const static interval empty {infinity,-infinity};
const static interval universe {-infinity, infinity};