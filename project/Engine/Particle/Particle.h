#pragma once
#include "TextureManager.h"
#include "MatrixCalculation.h"

enum SizeType {
	kNormal,
	kShift
};

enum SpeedType {
	kConstancy,
	kChange,
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

	Vector2 startSize = { 1.0f,1.0f };
	Vector2 endSize = { 1.0f,1.0f };

private:




private:


};
