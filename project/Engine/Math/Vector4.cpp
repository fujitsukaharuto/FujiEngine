#include"Vector4.h"
#include<cmath>

//長さ（ノルム）
float Length(const Vector4& v) {
	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

//正規化
Vector4 Normalize(const Vector4& v) {
	Vector4 result;

	float length = Length(v);
	if (length != 0) {
		result.x = v.x / length;
		result.y = v.y / length;
		result.z = v.z / length;
		result.w = v.w / length;
	}
	else {
		result.x = 0;
		result.y = 0;
		result.z = 0;
		result.w = 0;
	}
	return result;
}

// ベクトル加算
Vector4 Vector4::operator+(const Vector4& other) const {
	return Vector4(x + other.x, y + other.y, z + other.z, w + other.w);
}