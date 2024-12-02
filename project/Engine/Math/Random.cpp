#include "Random.h"

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
