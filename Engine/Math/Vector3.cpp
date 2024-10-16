#include "Vector3.h"

float Vector3::StaticLength(const Vector3& v){
	return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

float Vector3::Dot(const Vector3& other){
	return x * other.x + y * other.y + z * other.z;
}
