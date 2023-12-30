#ifndef VEC4_H
#define VEC4_H

#include <cmath>
#include <iostream> 
#include "vec3.h"

using std::sqrt;

class vec4 {
public:
    vec4() : e{ 0, 0, 0, 0 } {};
    vec4(double e0, double e1, double e2, double e3) : e{ e0, e1, e2, e3 } {};
    vec4(const vec3& v, double w=1) : e{ v.x(), v.y(), v.z(), w} {};

    double x() const { return e[0]; }
    double y() const { return e[1]; }
    double z() const { return e[2]; }
    double w() const { return e[3]; } // w-coordinate for homogeneous representation

    vec4 operator-() const { return vec4(-e[0], -e[1], -e[2], -e[3]); }
    double operator[](int i) const { return e[i]; }
    double& operator[](int i) { return e[i]; }

    vec4& operator+=(const vec4& v) {
        e[0] += v.e[0];
        e[1] += v.e[1];
        e[2] += v.e[2];
        e[3] += v.e[3];
        return *this;
    }

    vec4& operator*=(const double t) {
        e[0] *= t;
        e[1] *= t;
        e[2] *= t;
        e[3] *= t;
        return *this;
    }

    vec4& operator/=(const double t) {
        return *this *= 1 / t;
    }

    double length() const {
        return sqrt(length_squared());
    }

    double length_squared() const {
        return e[0] * e[0] + e[1] * e[1] + e[2] * e[2] + e[3] * e[3];
    }

    bool near_zero() const {
        const auto nearzero = 1e-8;
        return (fabs(e[0]) < nearzero) && (fabs(e[1]) < nearzero) && (fabs(e[2]) < nearzero) && (fabs(e[3]) < nearzero);
    }

    // Additional functions and random generation for vec4 can be added here

public:
    double e[4];
};

// Utility functions for vec4

inline std::ostream& operator<<(std::ostream& out, const vec4& v) {
    return out << v.e[0] << ' ' << v.e[1] << ' ' << v.e[2] << ' ' << v.e[3];
}

// Other operations like vector addition, subtraction, scalar multiplication, etc., can be defined similarly to vec3

#endif // VEC4_H
