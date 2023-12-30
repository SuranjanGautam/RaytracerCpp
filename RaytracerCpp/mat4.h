#pragma once

#include <cmath>
#include <stdexcept>
#include <initializer_list>
#include "vec4.h"

class mat4 {
public:
    mat4() : e{ {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0} } {};

    mat4(std::initializer_list<std::initializer_list<double>> list) {
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

    const double* operator[](int i) const {
        return e[i];
    }

    mat4& operator*=(const double t) {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                e[i][j] *= t;
            }
        }
        return *this;
    }

    mat4& operator/=(const double t) {
        *this *= (1 / t);
        return *this;
    }

    mat4 operator*(const mat4& m)
    {
        mat4 result;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                result[i][j] = 0;
                for (int k = 0; k < 4; ++k) {
                    result[i][j] += e[i][k] * m[k][j];
                }
            }
        }
        return result;
    }

    static mat4 identity() {
        mat4 i;
        for (int j = 0; j < 4; ++j) {
            i[j][j] = 1;
        }
        return i;
    }

    mat4 inverse() const {
        double det = determinant();
        if (det == 0) {
            throw std::runtime_error("Matrix is not invertible.");
        }

        mat4 adj;

        // Calculate the adjugate matrix (the transpose of the cofactor matrix)
        adj[0][0] = cofactor(0, 0);
        adj[0][1] = cofactor(1, 0);
        adj[0][2] = cofactor(2, 0);
        adj[0][3] = cofactor(3, 0);
        adj[1][0] = cofactor(0, 1);
        adj[1][1] = cofactor(1, 1);
        adj[1][2] = cofactor(2, 1);
        adj[1][3] = cofactor(3, 1);
        adj[2][0] = cofactor(0, 2);
        adj[2][1] = cofactor(1, 2);
        adj[2][2] = cofactor(2, 2);
        adj[2][3] = cofactor(3, 2);
        adj[3][0] = cofactor(0, 3);
        adj[3][1] = cofactor(1, 3);
        adj[3][2] = cofactor(2, 3);
        adj[3][3] = cofactor(3, 3);

        // Divide the adjugate by the determinant
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                adj[i][j] /= det;
            }
        }

        return adj;
    }

    static mat4 translation(const vec3& offset) {
        mat4 result = identity();
        result.e[0][3] = offset.x();
        result.e[1][3] = offset.y();
        result.e[2][3] = offset.z();
        return result;
    }

    static mat4 rotation_x(double theta) {
        mat4 result = identity();
        double c = cos(theta);
        double s = sin(theta);
        result.e[1][1] = c;
        result.e[1][2] = -s;
        result.e[2][1] = s;
        result.e[2][2] = c;
        return result;
    }

    static mat4 rotation_y(double theta) {
        mat4 result = identity();
        double c = cos(theta);
        double s = sin(theta);
        result.e[0][0] = c;
        result.e[0][2] = s;
        result.e[2][0] = -s;
        result.e[2][2] = c;
        return result;
    }

    static mat4 rotation_z(double theta) {
        mat4 result = identity();
        double c = cos(theta);
        double s = sin(theta);
        result.e[0][0] = c;
        result.e[0][1] = -s;
        result.e[1][0] = s;
        result.e[1][1] = c;
        return result;
    }

    static mat4 rotation(const vec3& rotation) {
        return rotation_z(rotation.z()) * rotation_y(rotation.y()) * rotation_x(rotation.x());
    }

    static mat4 scale(const vec3& scale) {
        mat4 result = identity();
        result.e[0][0] = scale.x();
        result.e[1][1] = scale.y();
        result.e[2][2] = scale.z();
        return result;
    }
private:
    double determinant() const {
        // Compute the determinant of the matrix
        double det = 0;
        for (int i = 0; i < 4; ++i) {
            det += (i % 2 == 0 ? 1 : -1) * e[0][i] * cofactor(0, i);
        }
        return det;
    }

    double cofactor(int row, int col) const {
        double minor[3][3];
        for (int i = 0, mi = 0; i < 4; ++i) {
            if (i == row) continue;
            for (int j = 0, mj = 0; j < 4; ++j) {
                if (j == col) continue;
                minor[mi][mj] = e[i][j];
                ++mj;
            }
            ++mi;
        }
        return ((row + col) % 2 == 0 ? 1 : -1) * determinant3x3(minor);
    }

    double determinant3x3(const double m[3][3]) const {
        return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
            m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
            m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
    }

    double e[4][4];
};

inline vec4 operator*(const mat4& m,const vec4& v) {
    return vec4(
        v[0] * m[0][0] + v[1] * m[0][1] + v[2] * m[0][2] + v[3] * m[0][3],
        v[0] * m[1][0] + v[1] * m[1][1] + v[2] * m[1][2] + v[3] * m[1][3],
        v[0] * m[2][0] + v[1] * m[2][1] + v[2] * m[2][2] + v[3] * m[2][3],
        v[0] * m[3][0] + v[1] * m[3][1] + v[2] * m[3][2] + v[3] * m[3][3]
    );
}

inline vec4 operator*(const vec4& v, const mat4& m) {
    return m * v;
}

inline mat4 operator*(const mat4& e, const mat4& m) {
    mat4 result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result[i][j] = 0;
            for (int k = 0; k < 4; ++k) {
                result[i][j] += e[i][k] * m[k][j];
            }
        }
    }
    return result;
}

vec3 toVec3(const vec4& v)
{
    if (v.w() != 0.0 && v.w() != 1.0) {
        return vec3(v.x() / v.w(), v.y() / v.w(), v.z() / v.w());
    }    
    return vec3(v.x(), v.y(), v.z());
}