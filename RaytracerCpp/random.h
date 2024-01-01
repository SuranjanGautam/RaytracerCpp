#pragma once

//#include <cstdlib>
inline double random_double()
{
	return (double)std::rand() / (RAND_MAX + 1);
}

inline double random_double(double min, double max)
{
	return min + ((max - min) * random_double());
}