#pragma once
#include "Math/Vector/Vector3.h"
#include "Math/Quaternion/Quaternion.h"

/// <summary>
/// キーフレーム構造体
/// </summary>
template <typename tValue>
struct Keyframe {
	float time;
	tValue value;
};
using KeyframeVector3 = Keyframe<Math::Vector3>;
using KeyframeQuaternion = Keyframe<Math::Quaternion>;