#pragma once
#include <map>
#include <string>
#include "NodeAnimation.h"

namespace Math {

	/// <summary>
	/// アニメーション構造体
	/// </summary>
	struct Animation {
		std::string name;
		float duration;
		std::map<std::string, NodeAnimation> nodeAnimations;
	};

}