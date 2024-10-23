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

	Vector3 GetCentarPos()const;

	bool GetLive()const { return isLive; }

	void SetLive(bool is) { isLive = is; }

private:



private:


	Object3d* enemy;
	bool isLive = true;

};
