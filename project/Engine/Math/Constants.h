#pragma once
#include <numbers>

namespace Math {

	//========================================================================*/
	//* 角度
	/// <summary>円周率</summary>
	inline constexpr float kPi = std::numbers::pi_v<float>;
	/// <summary>一周ぶんの角度</summary>
	inline constexpr float kTwoPi = kPi * 2.0f;
	/// <summary>直角</summary>
	inline constexpr float kHalfPi = kPi * 0.5f;

	/// <summary>度をラジアンに変換する</summary>
	inline constexpr float ToRadians(float degrees) { return degrees * (kPi / 180.0f); }
	/// <summary>ラジアンを度に変換する</summary>
	inline constexpr float ToDegrees(float radians) { return radians * (180.0f / kPi); }

}
