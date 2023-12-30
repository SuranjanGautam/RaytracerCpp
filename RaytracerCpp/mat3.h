#pragma once

#include <cmath>
#include "vec3.h"

class mat3 {
public:
	mat3() :e{ {0,0,0},{0,0,0},{0,0,0} } {};
	mat3(std::initializer_list<std::initializer_list<double>> list) {
		int row = 0;
		for (auto& r : list) {
			int col = 0;
			for (auto& val : r) {
				e[row][col] = val;
				++col;
			}
			++row;
		}
	}

	double* operator[](int i) {
		return e[i];
	}

	const double* operator[](int i) const{
		return e[i];
	}

	mat3& operator*=(const double t)
	{
		for (int i = 0; i < 3; ++i) {
			for (int j = 0; j < 3; ++j) {
				e[i][j] *= t;
			}
		}
		return *this;
	}

	mat3& operator/=(const double t)
	{
		*this *= (1/t);		
		return *this;
	}

	mat3 inverse() const {
		double det = determinant();
		if (det == 0) {
			throw std::runtime_error("Matrix is not invertible.");
		}

		mat3 adj;
		
		adj[0][0] = e[1][1] * e[2][2] - e[1][2] * e[2][1];
		adj[0][1] = e[0][2] * e[2][1] - e[0][1] * e[2][2];
		adj[0][2] = e[0][1] * e[1][2] - e[0][2] * e[1][1];
		adj[1][0] = e[1][2] * e[2][0] - e[1][0] * e[2][2];
		adj[1][1] = e[0][0] * e[2][2] - e[0][2] * e[2][0];
		adj[1][2] = e[0][2] * e[1][0] - e[0][0] * e[1][2];
		adj[2][0] = e[1][0] * e[2][1] - e[1][1] * e[2][0];
		adj[2][1] = e[0][1] * e[2][0] - e[0][0] * e[2][1];
		adj[2][2] = e[0][0] * e[1][1] - e[0][1] * e[1][0];
				
		adj /= det;

		return adj;
	}

	double determinant() const {		
		return e[0][0] * (e[1][1] * e[2][2] - e[1][2] * e[2][1]) -
			e[0][1] * (e[1][0] * e[2][2] - e[1][2] * e[2][0]) +
			e[0][2] * (e[1][0] * e[2][1] - e[1][1] * e[2][0]);
	}

	static mat3 identity() {
		static mat3 i = mat3();
		i[0][0] = 1;
		i[1][1] = 1;
		i[2][2] = 1;
		return i;
	}

	

	double e[3][3];
};

inline vec3 operator*(const vec3& v,const mat3& e)
{
	return vec3(
		v[0] * e[0][0] + v[1] * e[1][0] + v[2] * e[2][0],
		v[0] * e[0][0] + v[1] * e[1][1] + v[2] * e[2][1],
		v[0] * e[0][0] + v[1] * e[1][2] + v[2] * e[2][2]
	);
}

inline mat3 operator* (const mat3& m,const mat3& e)
{
	return mat3{
		{			
		{m[0][0] * e[0][0] + m[0][1] * e[1][0] + m[0][2] * e[2][0],
		m[0][0] * e[0][1] + m[0][1] * e[1][1] + m[0][2] * e[2][1],
		m[0][0] * e[0][2] + m[0][1] * e[1][2] + m[0][2] * e[2][2]},

		{m[1][0] * e[0][0] + m[1][1] * e[1][0] + m[1][2] * e[2][0],
		m[1][0] * e[0][1] + m[1][1] * e[1][1] + m[1][2] * e[2][1],
		m[1][0] * e[0][2] + m[1][2] * e[1][2] + m[1][2] * e[2][2]},

		{m[2][0] * e[0][0] + m[2][1] * e[1][0] + m[2][2] * e[2][0],
		m[2][0] * e[0][1] + m[2][1] * e[1][1] + m[2][2] * e[2][1],
		m[2][0] * e[0][2] + m[2][1] * e[1][2] + m[2][2] * e[2][2]}
	} };
}
