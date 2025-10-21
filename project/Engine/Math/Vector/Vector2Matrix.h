#pragma once
#include "Vector2.h"

/// <summary>
/// 4つ角構造体
/// </summary>
struct Quad final {
	Vector2 lt;
	Vector2 rt;
	Vector2 lb;
	Vector2 rb;
};