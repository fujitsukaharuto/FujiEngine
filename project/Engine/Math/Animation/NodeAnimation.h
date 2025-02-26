#pragma once
#include <vector>
#include "Keyframe.h"
#include "Vector3.h"
#include "Quaternion.h"

template <typename tValue>
struct AnimationCurve {
	std::vector<Keyframe<tValue>> keyframes;
};

struct NodeAnimation {
	AnimationCurve<Vector3> translate;
	AnimationCurve<Quaternion> rotate;
	AnimationCurve<Vector3> scale;
};