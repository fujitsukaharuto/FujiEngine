#pragma once
#include "Object3d.h"


class Enemy {
public:
	Enemy();
	~Enemy();

public:

	void Initialize();

	void Update();

	void Draw();

private:



private:


	Object3d* enemy;

};
