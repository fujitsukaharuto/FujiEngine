#pragma once
#include "Object3d.h"
#include "Sprite.h"

class Player {
public:
	Player();
	~Player();

public:

	void Initialize();

	void Update();

	void Draw();

	void SpriteDraw();

	Vector3 GetCenterBullet()const;
	Vector3 GetEndBullet()const { return velo_; }

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

	std::unique_ptr<Sprite> reticle;
};
