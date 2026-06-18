#include "piko/math.hpp"

#include "raylib.h"

#include <cmath>
#include <algorithm>

using namespace piko;

float Vect2::length() const{
    return std::sqrt((x*x) + (y*y));
}

Vect2 Vect2::normalize() const{
    float len = length();
    if(len != 0){
        return {
            x / len,
            y / len
        };
    }

    return {0};
}

bool Vect2::operator==(const Vect2& b) {
    return std::fabs(x - b.x) < P_EPSILON && std::fabs(y - b.y) < P_EPSILON;
}

Vect2::operator Vector2() const{
    return{x,y};
}

float Vect3::length() const{
    return std::sqrt((x*x) + (y*y) + (z*z));
}

Vect3 Vect3::normalize() const{
    float len = length();
    if(len != 0){
        return {
            x / len,
            y / len,
            z / len
        };
    }

    return {0};
}

bool Vect3::operator==(const Vect3& b){
    return std::fabs(x - b.x) < P_EPSILON &&
        std::fabs(y - b.y) < P_EPSILON &&
        std::fabs(z - b.z) < P_EPSILON;
}

Vect3::operator Vector3() const{
    return {x,y,z};
}

bool Rect::operator==(const Rect& b) const {
    return std::fabs(x - b.x) < P_EPSILON &&
           std::fabs(y - b.y) < P_EPSILON &&
           std::fabs(w - b.w) < P_EPSILON &&
           std::fabs(h - b.h) < P_EPSILON;
}

Rect::operator Rectangle() const{
    return {x, y, w, h};
}

Mat4::operator Matrix() const{
    Matrix mat;
    std::copy(m, m+16, &mat.m0);
    return mat;
}