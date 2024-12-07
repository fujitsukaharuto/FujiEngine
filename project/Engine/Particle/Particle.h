#pragma once
#include "TextureManager.h"
#include "MatrixCalculation.h"

enum SizeType {
	kNormal,
	kShift,
	kSin,
};

enum SpeedType {
	kConstancy,
	kChange,
};

enum ColorType {
	kDefault,
	kRandom,
};

class Particle {
public:
	Particle();
	~Particle();

public:

	bool isLive_ = false;
	float lifeTime_ = 0;
	float startLifeTime_ = 0;

	Vector3 accele{};
	Vector3 speed{};
	Trans transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{ 0.0f,0.0f,0.0f } };

	int type = SizeType::kNormal;
	int speedType = SpeedType::kConstancy;
	int colorType = ColorType::kDefault;

	Vector2 startSize = { 1.0f,1.0f };
	Vector2 endSize = { 1.0f,1.0f };

	Vector4 color = { 1.0f,1.0f,1.0f,1.0f };

	bool isBillBoard_ = true;

private:




private:


};
