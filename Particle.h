#pragma once
#include "MatrixCalculation.h"

enum SizeType {
	kNormal,
	kReduction,
	kExpantion,
};

enum SpeedType {
	kConstancy,
	kDecele,
	kAccele,
};

class Particle {
public:
	Particle();
	~Particle();

public:

	int lifeTime_ = 0;
	int startLifeTime_ = 0;
	bool isLive_ = false;
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
