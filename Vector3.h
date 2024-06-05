#pragma once
#include <cmath>

class Vector3
{
public:

	float x;
	float y;
	float z;

	//Constructor

	Vector3() = default;
	Vector3(float x, float y, float z) :x(x), y(y), z(z) {}
	Vector3(const Vector3& v) :x(v.x), y(v.y), z(v.z) {}

	//Operator

	Vector3& operator = (const Vector3& v) {
		x = v.x;
		y = v.y;
		z = v.z;
		return *this;
	}

	Vector3 operator+(const Vector3& v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
	Vector3 operator*(float k) const { return Vector3(x * k, y * k, z * k); }
	float operator*(const Vector3& v) const { return (x * v.x) + (y * v.y) + (z * v.z); }

};
