#include "Vector3.h"

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
	const float leng = Length();
	return leng == 0 ? *this : *this / leng;
}

Vector3 Vector3::Cross(const Vector3& other) const {
	return {
		y * other.z - z * other.y,
		z * other.x - x * other.z,
		x * other.y - y * other.x
	};
}