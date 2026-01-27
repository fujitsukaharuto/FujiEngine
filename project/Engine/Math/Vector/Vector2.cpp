#include "Vector2.h"

using namespace Math;


/*----- オペレーター -----------------------------------------------------------*/
Vector2& Vector2::operator = (const Vector2& v) {
	x = v.x;
	y = v.y;
	return *this;
}

Vector2& Vector2::operator += (const Vector2& v) {
	x += v.x;
	y += v.y;
	return *this;
}

Vector2& Vector2::operator -= (const Vector2& v) {
	x -= v.x;
	y -= v.y;
	return *this;
}

Vector2& Vector2::operator *= (float k) {
	x *= k;
	y *= k;
	return *this;
}

Vector2& Vector2::operator /= (float k) {
	x /= k;
	y /= k;
	return *this;
}



/*----- 関数 ------------------------------------------------------------------*/
Vector2 Vector2::Normalize() const {
	const float len = Length();
	return len == 0 ? *this : *this / len;
}