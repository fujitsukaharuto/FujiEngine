#pragma once
#include "Object3d.h"


class Enemy {
public:
	Enemy();
	~Enemy();

public:

	void Initialize(Vector3 pos, Vector3 speed);

	void Update();

	void Draw();

	Vector3 GetCentarPos()const;

	Vector3 GetVelocity()const { return velocity; }

	bool GetLive()const { return isLive; }

	void SetLive(bool is) { isLive = is; }

	void SetPosAndVelo(const Vector3& pos, const Vector3& speed);

	std::string GetModelName()const;

private:



private:

	Trans trans;
	Vector3 velocity;

	Object3d* enemy;
	bool isLive = true;

};
