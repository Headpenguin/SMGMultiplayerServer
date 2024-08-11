#ifndef VEC_HPP
#define VEC_HPP

#include <cmath>

struct Vec {
    float x, y, z;
    inline Vec() : x(0.0f), y(0.0f), z(0.0f) {}
    inline Vec(float x, float y, float z) : x(x), y(y), z(z) {}
    inline static Vec zero() {return Vec();}
    inline Vec operator+(const Vec &other) const {
        return Vec(x + other.x, y + other.y, z + other.z);
    }
    inline Vec operator-(const Vec &other) const {
        return Vec(x - other.x, y - other.y, z - other.z);
    }
    template<typename T>
    inline Vec operator*(T scalar) const {
        return Vec(x * scalar, y * scalar, z * scalar);
    }
    inline const Vec& operator+=(const Vec &other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }
    inline const Vec& operator-=(const Vec &other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }
    template<typename T>
    inline const Vec& operator*=(T scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }
    inline float dot(const Vec &other) const {
        return x * other.x + y * other.y + z * other.z;
    }
    inline float magnitude() const {
        return sqrt(dot(*this));
    }
    inline bool equal(const Vec &other, float tolerance) const {
        if((*this - other).magnitude() <= tolerance) return true;
        return false;
    }
    inline void setLength(float length) {
        *this *= length / magnitude();
    }
};


#endif
