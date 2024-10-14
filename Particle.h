#pragma once
#include "MatrixCalculation.h"


class Particle {
public:
	Particle();
	~Particle();

public:

	int lifeTime_ = 600;
	bool isLive_ = false;
	Vector3 accele{};
	Vector3 speed{};
	Trans transform{};

private:




private:


};
