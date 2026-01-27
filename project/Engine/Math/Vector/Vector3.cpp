#include "Vector3.h"

using namespace Math;


/*----- オペレーター -----------------------------------------------------------*/
Vector3& Vector3::operator = (const Vector3& v) {
	x = v.x;
	y = v.y;
	z = v.z;
	return *this;
}

Vector3& Vector3::operator += (const Vector3& v) {
	x += v.x;
	y += v.y;
	z += v.z;
	return *this;
}

Vector3& Vector3::operator -= (const Vector3& v) {
	x -= v.x;
	y -= v.y;
	z -= v.z;
	return *this;
}


/*----- 関数 ------------------------------------------------------------------*/
Vector3 Vector3::Normalize() const {
	const float len = Length();
	return len == 0 ? *this : *this / len;
}

Vector3 Vector3::Cross(const Vector3& other) const {
	return {
		y * other.z - z * other.y,
		z * other.x - x * other.z,
		x * other.y - y * other.x
	};
}