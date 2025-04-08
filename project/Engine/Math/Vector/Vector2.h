#pragma once
#include <cmath>

class Vector2 {
public:

	float x;
	float y;

	/*----- コンストラクタ ----------------------------------------------------------*/
	Vector2() : x(0), y(0) {};
	Vector2(float x, float y) :x(x), y(y) {}
	Vector2(const Vector2& v) :x(v.x), y(v.y) {}

	/*----- オペレーター -----------------------------------------------------------*/

	const Vector2& operator+() const {return *this;}

	Vector2 operator+() { return *this; }
	Vector2 operator-() const {return Vector2(-x, -y);}

	Vector2 operator * (float k) const { return Vector2(x * k, y * k); }
	Vector2 operator / (float k) const { return Vector2(x / k, y / k); }

	Vector2 operator + (const Vector2& v) const { return Vector2(x + v.x, y + v.y); }
	Vector2 operator - (const Vector2& v) const { return Vector2(x - v.x, y - v.y); }

	Vector2& operator = (const Vector2& v);
	Vector2& operator += (const Vector2& v);
	Vector2& operator -= (const Vector2& v);

	Vector2& operator *= (float k);
	Vector2& operator /= (float k);

	bool operator == (const Vector2& v) const { return (x == v.x && y == v.y); }
	bool operator != (const Vector2& v) const { return (x != v.x || y != v.y); }

	float operator * (const Vector2& v) const {return (x * v.x) + (y * v.y);}

	/*----- 関数 ------------------------------------------------------------------*/
	float Length() const { return std::sqrtf((*this) * (*this)); }
	float Angle() const { return std::atan2f(y, x); }
	Vector2 NormaliZe()const;

};