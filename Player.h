#pragma once
#include "Object3d.h"

class Player {
public:
	Player();
	~Player();

public:

	void Initialize();

	void Update();

	void Draw();

private:

	void ReticleCal();


private:


	Object3d* target;

	Object3d* bullet;
	Vector3 velo_;
};
