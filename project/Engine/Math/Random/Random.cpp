#include "Random.h"

#include <algorithm>
#include <cmath>
#include "Engine/Math/Constants.h"

using namespace Math;


std::mt19937 Random::generator_(std::random_device{}());

int Random::GetInt(int min, int max) {
	if (min > max) std::swap(min, max);
	std::uniform_int_distribution<int> dist(min, max);
	return dist(generator_);
}

float Random::GetFloat(float min, float max) {
	if (min > max) std::swap(min, max);
	std::uniform_real_distribution<float> dist(min, max);
	return dist(generator_);
}

Vector3 Random::GetVector3(const Vector2& x, const Vector2& y, const Vector2& z) {
	return Vector3({ {GetFloat(x.x,x.y)},{GetFloat(y.x,y.y)},{GetFloat(z.x,z.y)} });
}

Vector3 Random::GetVector3(const Vector3& min, const Vector3& max) {
	return Vector3(GetFloat(min.x, max.x), GetFloat(min.y, max.y), GetFloat(min.z, max.z));
}

bool Random::GetBool(float trueProbability) {
	if (trueProbability <= 0.0f) { return false; }
	if (trueProbability >= 1.0f) { return true; }
	return GetFloat(0.0f, 1.0f) < trueProbability;
}

Vector3 Random::GetOnUnitSphere() {
	// z を一様に取ってから緯度を決めると球面上で一様になる(成分ごとの乱数だと角に偏る)
	const float z = GetFloat(-1.0f, 1.0f);
	const float theta = GetFloat(0.0f, kTwoPi);
	const float r = std::sqrt((std::max)(0.0f, 1.0f - z * z));
	return Vector3(r * std::cos(theta), r * std::sin(theta), z);
}

Vector2 Random::GetInsideCircle(float radius) {
	// 半径に sqrt を掛けないと中心に密集する
	const float theta = GetFloat(0.0f, kTwoPi);
	const float r = radius * std::sqrt(GetFloat(0.0f, 1.0f));
	return Vector2(r * std::cos(theta), r * std::sin(theta));
}
