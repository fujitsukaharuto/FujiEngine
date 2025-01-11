#pragma once
#include "Math/MatrixCalculation.h"

class Camera;
class GameCamera {
public:
	GameCamera();
	~GameCamera();

public:

	void Initialize();
	void Update();

	void SetTarget(const Trans* target);
	void Reset();
	void Debug();

	Vector3 OffsetCal() const;

private:


private:

	const Trans* target_ = nullptr;
	Vector3 interTarget_ = {};
	float destinationAngleY_ = 0;
	Vector3 offset_;
	Camera* camera_;
};
