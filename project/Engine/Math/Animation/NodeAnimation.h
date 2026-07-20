#pragma once
#include <vector>
#include "Keyframe.h"
#include "Engine/Math/Vector/Vector3.h"
#include "Engine/Math/Quaternion/Quaternion.h"

namespace Math {

	/// <summary>
	/// キーフレームを並べたアニメーションカーブ
	/// </summary>
	template <typename tValue>
	struct AnimationCurve {
		std::vector<Keyframe<tValue>> keyframes;
	};

	/// <summary>
	/// ノードアニメーション構造体
	/// </summary>
	struct NodeAnimation {
		AnimationCurve<Math::Vector3> translate;
		AnimationCurve<Math::Quaternion> rotate;
		AnimationCurve<Math::Vector3> scale;
	};

}