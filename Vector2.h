#pragma once
#include <cmath>

class Vector2
{
public:

	float x;
	float y;

	//Constructor

	Vector2() = default;
	Vector2(float x, float y) :x(x), y(y) {}
	Vector2(const Vector2& v) :x(v.x), y(v.y) {}

	//Operator

	Vector2& operator = (const Vector2& v) {
		x = v.x;
		y = v.y;
		return *this;
	}

	bool operator == (const Vector2& v) const {
		return (x == v.x && y == v.y);
	}

	bool operator != (const Vector2& v) const {
		return (x != v.x || y != v.y);
	}

	const Vector2& operator+() const {
		return *this;
	}

	Vector2& operator+() {
		return *this;
	}

	Vector2 operator-() const {
		return Vector2(-x, -y);
	}

	Vector2& operator += (const Vector2& v) {
		x += v.x;
		y += v.y;
		return *this;
	}

	Vector2 operator + (const Vector2& v) const {
		return Vector2(x + v.x, y + v.y);
	}

	Vector2& operator -= (const Vector2& v) {
		x -= v.x;
		y -= v.y;
		return *this;
	}

	Vector2 operator - (const Vector2& v) const {
		return Vector2(x - v.x, y - v.y);
	}

	Vector2& operator *= (float k) {
		x *= k;
		y *= k;
		return *this;
	}

	Vector2 operator * (float k) const {
		return Vector2(x * k, y * k);
	}

	Vector2& operator /= (float k) {
		x /= k;
		y /= k;
		return *this;
	}

	Vector2 operator / (float k)const {
		return Vector2(x / k, y / k);
	}

	float operator * (const Vector2& v) const {
		return (x * v.x) + (y * v.y);
	}

	//Function

	float Lenght() const {
		return std::sqrtf((*this) * (*this));
	}

	Vector2 NormaliZe()const {
		const float leng = Lenght();
		return leng == 0 ? *this : *this / leng;
	}

	float Angle() const {
		return std::atan2f(y, x);
	}
};