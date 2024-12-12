#pragma once
#include <cmath>

class Vector3 {
public:

	float x;
	float y;
	float z;

	//Constructor

	Vector3() : x(0), y(0), z(0) {};
	Vector3(float x, float y, float z) :x(x), y(y), z(z) {}
	Vector3(const Vector3& v) :x(v.x), y(v.y), z(v.z) {}

	//Operator

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

	float Dot(const Vector3& other)const { return *this * other; }

	Vector3 Cross(const Vector3& other) const {
		return {
			y * other.z - z * other.y,
			z * other.x - x * other.z,
			x * other.y - y * other.x
		};
	}

	static Vector3 GetZeroVec() { return { 0.0f,0.0f,0.0f }; }
	static Vector3 GetUpVec() { return { 0.0f,1.0f,0.0f }; }
	static float Dot(const Vector3& a, const Vector3& b) { return a * b; }
	static float Length(const Vector3& a) { return a.Length(); }

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

