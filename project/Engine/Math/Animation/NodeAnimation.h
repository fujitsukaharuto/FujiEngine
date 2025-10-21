#pragma once
#include <vector>
#include "Keyframe.h"
#include "Math/Vector/Vector3.h"
#include "Math/Quaternion/Quaternion.h"

template <typename tValue>
struct AnimationCurve {
	std::vector<Keyframe<tValue>> keyframes;
};

/// <summary>
/// ノードアニメーション構造体
/// </summary>
struct NodeAnimation {
	AnimationCurve<Vector3> translate;
	AnimationCurve<Quaternion> rotate;
	AnimationCurve<Vector3> scale;
};