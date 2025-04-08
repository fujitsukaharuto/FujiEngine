#pragma once
#include <random>
#include "Math/Vector/Vector3.h"
#include "Math/Vector/Vector2.h"

class Random {
public:
	Random() = default;
	~Random() = default;

public:

	static int GetInt(int min, int max);

	static float GetFloat(float min, float max);


	/// <summary>
	/// Vector3のランダムの値
	/// </summary>
	/// <param name="x">の{ min, max }</param>
	/// <param name="y">の{ min, max }</param>
	/// <param name="z">の{ min, max }</param>
	/// <returns></returns>
	static Vector3 GetVector3(const Vector2& x, const Vector2& y, const Vector2& z);

private:


private:

	static std::mt19937 generator_;

};
