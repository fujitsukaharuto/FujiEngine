#pragma once
#include "Object3d.h"


class Enemy {
public:
	Enemy();
	~Enemy();

public:

	void Initialize(Vector3 pos, Vector3 speed, int modelNum);

	void Update();

	void Draw();

	Vector3 GetCentarPos()const;

	Vector3 GetVelocity()const { return velocity; }

	bool GetLive()const { return isLive; }

	void SetLive(bool is) { isLive = is; }

	void SetPosAndVelo(const Vector3& pos, const Vector3& speed);

	std::string GetModelName()const;

	int GetModelNum() { return modelN; }

	int GetScore() { return score; }

	int life = 0;

private:



private:

	Trans trans;
	Vector3 velocity;

	Object3d* enemy;
	bool isLive = true;
	float liveTime_ = 900.0f;

	int score = 60;
	int modelN = 0;
};
