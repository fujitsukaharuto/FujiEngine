#include "Vector2.h"

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
Vector2 Vector2::NormaliZe() const {
	const float leng = Length();
	return leng == 0 ? *this : *this / leng;
}