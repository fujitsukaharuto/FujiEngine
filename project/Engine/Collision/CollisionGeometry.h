#pragma once
#include <array>
#include "Engine/Math/Vector/Vector3.h"

namespace Collision {

	/// <summary>
	/// 向きのある箱
	/// </summary>
	struct OBB {
		Math::Vector3 size;		// ハーフサイズ
		Math::Vector3 center;
		/// <summary>正規直交基底。ワールド行列から直接取る(オイラー角を経由すると非可逆で判定がズレる)</summary>
		std::array<Math::Vector3, 3> axes;
	};

	/// <summary>
	/// 球
	/// </summary>
	struct Sphere {
		Math::Vector3 center;
		float radius = 0.0f;
	};

	/// <summary>
	/// カプセル。線分 a-b から radius 以内
	/// </summary>
	struct Capsule {
		Math::Vector3 a;
		Math::Vector3 b;
		float radius = 0.0f;
	};

	/// <summary>
	/// レイ。direction は単位ベクトル
	/// </summary>
	struct Ray {
		Math::Vector3 origin;
		Math::Vector3 direction;
	};

	/// <summary>
	/// レイが当たった所
	/// </summary>
	struct RayHit {
		float distance = 0.0f;
		Math::Vector3 point;
		Math::Vector3 normal;	// 当たった面の外向き
	};

	/// <summary>形どうしの重なりとレイの当たり。エンジンの他の部分に依存しない</summary>
	namespace Geometry {

		/// <summary>線分 a-b の上で p にいちばん近い点</summary>
		Math::Vector3 ClosestPointOnSegment(const Math::Vector3& p, const Math::Vector3& a, const Math::Vector3& b);

		/// <summary>箱の中 (表面を含む) で p にいちばん近い点</summary>
		Math::Vector3 ClosestPointOnOBB(const Math::Vector3& p, const OBB& obb);

		/// <summary>2本の線分のいちばん近い点の組</summary>
		/// <returns>その2点の距離の2乗</returns>
		float ClosestPointsSegmentSegment(const Math::Vector3& a1, const Math::Vector3& b1,
			const Math::Vector3& a2, const Math::Vector3& b2, Math::Vector3& onFirst, Math::Vector3& onSecond);

		/// <summary>線分と箱のいちばん近い距離の2乗。交わっていれば 0</summary>
		float SegmentOBBDistanceSq(const Math::Vector3& a, const Math::Vector3& b, const OBB& obb);

		bool Intersects(const OBB& a, const OBB& b);
		bool Intersects(const Sphere& a, const Sphere& b);
		bool Intersects(const Sphere& sphere, const OBB& obb);
		bool Intersects(const Capsule& capsule, const Sphere& sphere);
		bool Intersects(const Capsule& a, const Capsule& b);
		bool Intersects(const Capsule& capsule, const OBB& obb);

		/// <remarks>原点が中にあるときは当たらない (Unity の Physics.Raycast と同じ)</remarks>
		bool Raycast(const Ray& ray, const Sphere& sphere, float maxDistance, RayHit& hit);
		/// <remarks>原点が中にあるときは当たらない</remarks>
		bool Raycast(const Ray& ray, const OBB& obb, float maxDistance, RayHit& hit);
		/// <remarks>原点が中にあるときは当たらない</remarks>
		bool Raycast(const Ray& ray, const Capsule& capsule, float maxDistance, RayHit& hit);
	}

}
