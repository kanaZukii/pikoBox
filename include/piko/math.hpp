// License: MIT
// Copyright (c) 2026 kanaZukii (GelBanana)

#pragma once

#include <cmath>
#include <cstdint>

// Math structs for conversion with Raylib
struct Matrix;
struct Vector2;
struct Vector3;
struct Rectangle;
struct Color;

namespace piko {

    constexpr float P_EPSILON = 1e-6f;
    constexpr float P_PI = 3.14159265358979323846f;
    constexpr float P_TWO_PI  = 6.28318530717958647692f;
    constexpr float P_HALF_PI = 1.57079632679489661923f;

    constexpr float P_DEG2RAD = P_PI / 180.0f;
    constexpr float P_RAD2DEG = 180.0f / P_PI;

    struct Vect2
    {
        union {
            struct {
                float x, y;
            };
            float v[2];
        };

        Vect2() : x(0), y(0) {}

        Vect2(float n) : x(n), y(n) {}

        Vect2(float x, float y) : x(x), y(y) {}
        
        float length() const;

        float lengthSquared() const { return x * x + y * y; }

        Vect2 normalize() const;

        float& operator[](int i) { return v[i]; }
        float operator[](int i) const { return v[i]; }

        bool operator==(const Vect2& b);

        operator Vector2() const;
    };

    struct Vect3
    {
        union {
            struct {
                float x, y, z;
            };
            float v[3];
        };

        Vect3() : x(0), y(0), z(0) {}

        Vect3(float n) : x(n), y(n), z(n) {} 

        Vect3(float x, float y, float z) : x(x), y(y), z(z) {}

        float length() const;

        Vect3 normalize() const;

        float& operator[](int i) { return v[i]; }
        float operator[](int i) const { return v[i]; }

        bool operator==(const Vect3& b);

        operator Vector3() const;
    };

    struct Rect
    {
        union{
            struct {
                float x,y,w,h;
            };
            float r[4];
        };

        Rect() : x(0), y(0), w(0), h(0) {}
        Rect(float n) : x(n), y(n), w(n), h(n) {}
        Rect(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}

        float& operator[](int i) { return r[i]; }
        float operator[](int i) const { return r[i]; }

        bool operator==(const Rect& b) const;

        operator Rectangle() const;
    };

    struct Color4 {
         union{
            struct {
                uint8_t r,g,b,a;
            };
            uint8_t c[4];
        };
    };

    struct Color3 {
        union{
            struct {
                uint8_t r,g,b;
            };
            uint8_t c[3];
        };
    };
    

    struct Mat4 {
        union {
            struct {
                float m0, m4, m8, m12; 
                float m1, m5, m9, m13; 
                float m2, m6, m10, m14;
                float m3, m7, m11, m15;
            };
            float m[16];
        };

        Mat4() { *this = Identity(); }

        Mat4(float v0, float v4, float v8, float v12,
            float v1, float v5, float v9, float v13,
            float v2, float v6, float v10, float v14,
            float v3, float v7, float v11, float v15) 
            : m0(v0), m4(v4), m8(v8), m12(v12),
            m1(v1), m5(v5), m9(v9), m13(v13),
            m2(v2), m6(v6), m10(v10), m14(v14),
            m3(v3), m7(v7), m11(v11), m15(v15) {}

        static Mat4 Identity() {
            return {
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
            };
        }

        static Mat4 Zero() {
            return {
                0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f
            };
        }

        static Mat4 Ortho(float left, float right, float bottom, float top, float znear, float zfar) {
            Mat4 res = Identity();
            
            float rl = (right - left);
            float tb = (top - bottom);
            float fn = (zfar - znear);

            res.m0  = 2.0f / rl;
            res.m5  = 2.0f / tb;
            res.m10 = -2.0f / fn;

            res.m12 = -(right + left) / rl;
            res.m13 = -(top + bottom) / tb;
            res.m14 = -(zfar + znear) / fn;

            return res;
        }

        static Mat4 View2D(Vect2 offset, Vect2 target, float rotation, float zoom) {
            Mat4 mat = Mat4::Identity();

            // Convert rotation to radians (using our P_PI or RAD2DEG)
            float rad = rotation * P_DEG2RAD;
            float cosR = std::cos(rad);
            float sinR = std::sin(rad);

            // Setup the combined rotation and scale components
            mat.m0 = cosR * zoom;
            mat.m1 = sinR * zoom;
            mat.m4 = -sinR * zoom;
            mat.m5 = cosR * zoom;

            // Calculate the translation components (the "view" position)
            mat.m12 = offset.x - (mat.m0 * target.x + mat.m4 * target.y);
            mat.m13 = offset.y - (mat.m1 * target.x + mat.m5 * target.y);

            return mat;
        }

        operator Matrix() const;
    };

    inline Vect2 TransformVect2(const Vect2& v, const Mat4& mat) {
        Vect2 result;
        // Standard Vector4 * Mat4 logic, simplified for 2D (z=0, w=1)
        result.x = v.x * mat.m0 + v.y * mat.m4 + mat.m12;
        result.y = v.x * mat.m1 + v.y * mat.m5 + mat.m13;
        return result;
    }
}