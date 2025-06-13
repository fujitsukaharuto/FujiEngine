#pragma once
#include "Engine/Math/Matrix/MatrixCalculation.h"


class FollowCamera {
public:
	FollowCamera();
	~FollowCamera();

public:

	void Initialize();
	void Update(const Vector3& lockon);

	void SetTarget(const Trans* target);
	void Reset();


	Vector3 OffsetCal() const;

private:


private:

	const Trans* target_ = nullptr;
	Vector3 interTarget_ = {};
	float destinationAngleY_ = 0;

};