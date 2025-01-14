#pragma once
#include "Math/Vector3.h"
#include "Math/Quaternion.h"


struct KeyframeVector3 {
	Vector3 value;
	float time;
};

struct keyframeQuaternion {
	Quaternion value;
	float time;
};