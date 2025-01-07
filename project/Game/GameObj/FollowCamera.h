#pragma once
#include "Math/MatrixCalculation.h"

class LockOn;

class FollowCamera {
public:
	FollowCamera();
	~FollowCamera();

public:

	void Initialize();
	void Update();

	void SetTarget(const Trans* target);
	void Reset();

	void SetLockOn(const LockOn* target);


	Vector3 OffsetCal() const;

private:


private:

	const Trans* target_ = nullptr;
	Vector3 interTarget_ = {};
	float destinationAngleY_ = 0;
	const LockOn* lockOn_ = nullptr;

};
