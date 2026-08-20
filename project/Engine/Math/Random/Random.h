#pragma once
#include <random>
#include "Engine/Math/Vector/Vector3.h"
#include "Engine/Math/Vector/Vector2.h"

namespace Math {
	/// <summary>
	/// ランダム生成クラス
	/// </summary>
	class Random {
	public:
		Random() = default;
		~Random() = default;

	public:

		/// <summary>Int型のランダム生成</summary>
		static int GetInt(int min, int max);

		/// <summary>Float型のランダム生成</summary>
		static float GetFloat(float min, float max);


		/// <summary>
		/// Vector3のランダムの値
		/// </summary>
		/// <param name="x">の{ min, max }</param>
		/// <param name="y">の{ min, max }</param>
		/// <param name="z">の{ min, max }</param>
		/// <returns></returns>
		static Vector3 GetVector3(const Vector2& x, const Vector2& y, const Vector2& z);

		/// <summary>各成分を min〜max の範囲で決めた Vector3</summary>
		/// <remarks>軸ごとに { min, max } を書く上のオーバーロードより、範囲を2点で書きたいとき用</remarks>
		static Vector3 GetVector3(const Vector3& min, const Vector3& max);

		/// <summary>指定した確率で true を返す</summary>
		/// <param name="trueProbability">0.0で必ずfalse、1.0で必ずtrue</param>
		static bool GetBool(float trueProbability = 0.5f);

		/// <summary>単位球面上の一様な1点。長さは常に1</summary>
		/// <remarks>全方向へ均等に飛ばしたいとき用。成分ごとに乱数を振ると角に偏るのでこれを使うこと</remarks>
		static Vector3 GetOnUnitSphere();

		/// <summary>半径 radius の円の内側の一様な1点(XY平面)</summary>
		static Vector2 GetInsideCircle(float radius = 1.0f);

	private:


	private:

		static std::mt19937 generator_;

	};
}