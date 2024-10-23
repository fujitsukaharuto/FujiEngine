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

	Vector3 GetCenterBullet()const;

	float GetBulletScale()const;

	bool GetLive()const { return isLive; }

	void SetLive(bool is) { isLive = is; }

private:

	void ReticleCal();


private:


	Object3d* target;

	Object3d* bullet;
	bool isLive = false;
	float startTime = 0.0f;
	const float kStartTime = 20.0f;
	Vector3 velo_;
};
