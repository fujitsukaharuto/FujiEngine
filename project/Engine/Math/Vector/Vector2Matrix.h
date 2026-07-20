#pragma once
#include "Vector2.h"

namespace Math {

	/// <summary>
	/// 4つ角構造体
	/// </summary>
	struct Quad final {
		Math::Vector2 lt;
		Math::Vector2 rt;
		Math::Vector2 lb;
		Math::Vector2 rb;
	};

}