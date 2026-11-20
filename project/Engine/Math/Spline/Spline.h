#pragma once
#include <cstdint>
#include <vector>
#include "Engine/Math/Vector/Vector3.h"

namespace Math {

	/// <summary>
	/// 点を通る曲線 (UE の Spline Component / Unity の Splines と同じ使い方)
	/// </summary>
	/// <remarks>
	/// 区間は Hermite 曲線。接線は前後の点から自動で決める (一様な Catmull-Rom と同じ。UE の Curve 点の自動接線)。
	/// 位置は「キー」(0 〜 区間の数、整数が点) か、始点からの「距離」で引く。距離はキーとの対応表 (UE の reparameterization table) で引く
	/// </remarks>
	class Spline {
	public:
		/// <summary>その点から次の点までの区間のつなぎ方</summary>
		enum class PointType : uint8_t {
			Curve,	// 自動の接線で滑らかに
			Linear,	// 直線
		};

		struct Point {
			Vector3 position{};
			PointType type = PointType::Curve;
		};

		/// <summary>点を入れて距離の表を作り直す</summary>
		/// <param name="isClosed">最後の点から最初の点へもつなぐ</param>
		void Set(const std::vector<Point>& points, bool isClosed);

		const std::vector<Point>& GetPoints() const { return points_; }
		bool IsClosed() const { return isClosed_; }
		/// <summary>区間の数。点が2つ未満なら0</summary>
		int GetSegmentCount() const;

		/// <summary>キーの位置。キーは 0 〜 GetSegmentCount() で、整数が点</summary>
		Vector3 Evaluate(float key) const;
		/// <summary>キーでの微分 (向きと速さ)</summary>
		Vector3 EvaluateDerivative(float key) const;

		/// <summary>曲線の長さ</summary>
		float GetLength() const { return lengths_.empty() ? 0.0f : lengths_.back(); }
		/// <summary>始点から distance 進んだ所のキー。閉じた曲線は一周で折り返し、開いた曲線は端で止まる</summary>
		float GetKeyAtDistance(float distance) const;
		Vector3 GetLocationAtDistance(float distance) const;
		/// <summary>進む向き (単位ベクトル)</summary>
		Vector3 GetDirectionAtDistance(float distance) const;
		/// <summary>キーまでの距離</summary>
		float GetDistanceAtKey(float key) const;
		/// <summary>いちばん近い所のキー (UE の FindInputKeyClosestToWorldLocation)</summary>
		float FindClosestKey(const Vector3& position) const;

	private:
		/// <summary>点の接線 (Catmull-Rom)。開いた曲線の端は隣の点への差</summary>
		Vector3 TangentAt(int index) const;
		/// <summary>キーを区間の番号と区間の中の位置 (0〜1) へ</summary>
		void SplitKey(float key, int& segment, float& t) const;
		/// <summary>表のキーの刻みと距離を作る</summary>
		void BuildLengthTable();

	private:
		// 1区間を何等分して距離の表を作るか (UE の ReparamStepsPerSegment は 10)
		static constexpr int kStepsPerSegment_ = 16;

		std::vector<Point> points_;
		bool isClosed_ = false;
		// キー i / kStepsPerSegment_ までの距離
		std::vector<float> lengths_;
	};

}
