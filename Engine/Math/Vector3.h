#pragma once
#include <cmath>
#include <json.hpp>

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

	bool operator==(const Vector3& other) const {
		const float epsilon = 1e-5f; // 許容誤差の定義
		return (std::fabs(x - other.x) < epsilon) &&
			(std::fabs(y - other.y) < epsilon) &&
			(std::fabs(z - other.z) < epsilon);
	}

	Vector3& operator = (const Vector3& v) {
		x = v.x;
		y = v.y;
		z = v.z;
		return *this;
	}

	Vector3 operator+()const { return *this; }
	Vector3 operator-()const { return Vector3(-x, -y, -z); }

	Vector3 operator+(const Vector3& v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
	Vector3& operator += (const Vector3& v) {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}
	
	Vector3 operator-(const Vector3& v)const { return Vector3(x - v.x, y - v.y, z - v.z); }
	Vector3& operator -= (const Vector3& v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}
	
	Vector3 operator*(float k) const { return Vector3(x * k, y * k, z * k); }
	float operator*(const Vector3& v) const { return (x * v.x) + (y * v.y) + (z * v.z); }
	
	Vector3 operator/(float k) const { return Vector3(x / k, y / k, z / k); }


	float Length() const { return std::sqrtf((*this) * (*this)); }
	Vector3 Normalize() const {
		const float leng = Length();
		return leng == 0 ? *this : *this / leng;
	}

};





inline Vector3 operator+(float k, const Vector3& v) {
	return Vector3(k + v.x, k + v.y, k + v.z);
}

inline Vector3 operator-(float k, const Vector3& v) {
	return Vector3(k - v.x, k - v.y, k - v.z);
}

inline Vector3 operator*(float k, const Vector3& v) {
	return Vector3(k * v.x, k * v.y, k * v.z);
}

