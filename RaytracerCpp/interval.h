#pragma once

#include "general.h"
class interval {
public:
	double min, max;


	interval():min(infinity),max(-infinity) {}; //empty
	interval(double _min, double _max) :min(_min), max(_max) {};

	bool contains(double value) const {
		return value >= min && value <= max;
	}

	bool sorrounds(double value) const {
		return value > min && value < max;
	}

	double clamp(double value) const {
		return std::min(std::max(value, min), max);
	}

	static const interval empty, universe;
};

const static interval empty {infinity,-infinity};
const static interval universe {-infinity, infinity};