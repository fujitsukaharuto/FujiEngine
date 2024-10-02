#pragma once
#include <cmath>

class MyVec2
{
public:

	float x;
	float y;

	//Constructor

	MyVec2() = default;
	MyVec2(float x, float y) :x(x), y(y) {}
	MyVec2(const MyVec2& v) :x(v.x), y(v.y) {}

	//Operator

	MyVec2& operator = (const MyVec2& v) {
		x = v.x;
		y = v.y;
		return *this;
	}

	bool operator == (const MyVec2& v) const {
		return (x == v.x && y == v.y);
	}

	bool operator != (const MyVec2& v) const {
		return (x != v.x || y != v.y);
	}

	const MyVec2& operator+() const {
		return *this;
	}

	MyVec2& operator+() {
		return *this;
	}

	MyVec2 operator-() const {
		return MyVec2(-x, -y);
	}

	MyVec2& operator += (const MyVec2& v) {
		x += v.x;
		y += v.y;
		return *this;
	}

	MyVec2 operator + (const MyVec2& v) const {
		return MyVec2(x + v.x, y + v.y);
	}

	MyVec2& operator -= (const MyVec2& v) {
		x -= v.x;
		y -= v.y;
		return *this;
	}

	MyVec2 operator - (const MyVec2& v) const {
		return MyVec2(x - v.x, y - v.y);
	}

	MyVec2& operator *= (float k) {
		x *= k;
		y *= k;
		return *this;
	}

	MyVec2 operator * (float k) const {
		return MyVec2(x * k, y * k);
	}

	MyVec2& operator /= (float k) {
		x /= k;
		y /= k;
		return *this;
	}

	MyVec2 operator / (float k)const {
		return MyVec2(x / k, y / k);
	}

	float operator * (const MyVec2& v) const {
		return (x * v.x) + (y * v.y);
	}

	//Function

	float Lenght() const {
		return std::sqrtf((*this) * (*this));
	}

	MyVec2 NormaliZe()const {
		const float leng = Lenght();
		return leng == 0 ? *this : *this / leng;
	}

	float Angle() const {
		return std::atan2f(y, x);
	}
};