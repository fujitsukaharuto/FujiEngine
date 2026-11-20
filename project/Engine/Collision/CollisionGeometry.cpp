#include "CollisionGeometry.h"
#include <algorithm>
#include <cmath>
#include <limits>

using namespace Math;

namespace Collision::Geometry {

	namespace {
		constexpr float kEpsilon = 1e-6f;

		/// <summary>箱の中心から見た p を箱の軸で表す</summary>
		Vector3 ToLocal(const Vector3& p, const OBB& obb) {
			const Vector3 d = p - obb.center;
			return { Vector3::Dot(d, obb.axes[0]), Vector3::Dot(d, obb.axes[1]), Vector3::Dot(d, obb.axes[2]) };
		}

		/// <summary>箱の座標系の点と、ハーフサイズの箱との距離の2乗</summary>
		float LocalBoxDistanceSq(const Vector3& p, const Vector3& half) {
			const float dx = (std::max)(std::abs(p.x) - half.x, 0.0f);
			const float dy = (std::max)(std::abs(p.y) - half.y, 0.0f);
			const float dz = (std::max)(std::abs(p.z) - half.z, 0.0f);
			return dx * dx + dy * dy + dz * dz;
		}

		/// <summary>球の表面とレイ。原点が中にあれば当たらない</summary>
		bool RaySphereSurface(const Vector3& origin, const Vector3& direction, const Vector3& center, float radius, float& t) {
			const Vector3 m = origin - center;
			const float b = Vector3::Dot(m, direction);
			const float c = Vector3::Dot(m, m) - radius * radius;
			if (c <= 0.0f || b > 0.0f) {
				return false;
			}
			const float discriminant = b * b - c;
			if (discriminant < 0.0f) {
				return false;
			}
			t = -b - std::sqrt(discriminant);
			return t >= 0.0f;
		}
	}

	Vector3 ClosestPointOnSegment(const Vector3& p, const Vector3& a, const Vector3& b) {
		const Vector3 ab = b - a;
		const float lengthSq = Vector3::Dot(ab, ab);
		if (lengthSq <= kEpsilon) {
			return a;
		}
		const float t = std::clamp(Vector3::Dot(p - a, ab) / lengthSq, 0.0f, 1.0f);
		return a + ab * t;
	}

	Vector3 ClosestPointOnOBB(const Vector3& p, const OBB& obb) {
		const Vector3 local = ToLocal(p, obb);
		const float x = std::clamp(local.x, -obb.size.x, obb.size.x);
		const float y = std::clamp(local.y, -obb.size.y, obb.size.y);
		const float z = std::clamp(local.z, -obb.size.z, obb.size.z);
		return obb.center + obb.axes[0] * x + obb.axes[1] * y + obb.axes[2] * z;
	}

	// Ericson, Real-Time Collision Detection 5.1.9
	float ClosestPointsSegmentSegment(const Vector3& a1, const Vector3& b1, const Vector3& a2, const Vector3& b2,
		Vector3& onFirst, Vector3& onSecond) {
		const Vector3 d1 = b1 - a1;
		const Vector3 d2 = b2 - a2;
		const Vector3 r = a1 - a2;
		const float a = Vector3::Dot(d1, d1);
		const float e = Vector3::Dot(d2, d2);
		const float f = Vector3::Dot(d2, r);
		float s = 0.0f;
		float t = 0.0f;
		if (a <= kEpsilon && e <= kEpsilon) {
			onFirst = a1;
			onSecond = a2;
			return Vector3::Dot(a1 - a2, a1 - a2);
		}
		if (a <= kEpsilon) {
			t = std::clamp(f / e, 0.0f, 1.0f);
		} else {
			const float c = Vector3::Dot(d1, r);
			if (e <= kEpsilon) {
				s = std::clamp(-c / a, 0.0f, 1.0f);
			} else {
				const float b = Vector3::Dot(d1, d2);
				const float denominator = a * e - b * b;
				// 平行なら s = 0
				s = (denominator > kEpsilon) ? std::clamp((b * f - c * e) / denominator, 0.0f, 1.0f) : 0.0f;
				t = (b * s + f) / e;
				if (t < 0.0f) {
					t = 0.0f;
					s = std::clamp(-c / a, 0.0f, 1.0f);
				} else if (t > 1.0f) {
					t = 1.0f;
					s = std::clamp((b - c) / a, 0.0f, 1.0f);
				}
			}
		}
		onFirst = a1 + d1 * s;
		onSecond = a2 + d2 * t;
		const Vector3 d = onFirst - onSecond;
		return Vector3::Dot(d, d);
	}

	float SegmentOBBDistanceSq(const Vector3& a, const Vector3& b, const OBB& obb) {
		const Vector3 localA = ToLocal(a, obb);
		const Vector3 localB = ToLocal(b, obb);
		const Vector3 delta = localB - localA;
		// 箱との距離は t について下に凸。黄金分割で最小を探す
		auto distanceAt = [&](float t) { return LocalBoxDistanceSq(localA + delta * t, obb.size); };
		constexpr float kInvPhi = 0.6180339887f;
		float lo = 0.0f;
		float hi = 1.0f;
		float x1 = hi - (hi - lo) * kInvPhi;
		float x2 = lo + (hi - lo) * kInvPhi;
		float f1 = distanceAt(x1);
		float f2 = distanceAt(x2);
		for (int i = 0; i < 40; i++) {
			if (f1 <= f2) {
				hi = x2;
				x2 = x1;
				f2 = f1;
				x1 = hi - (hi - lo) * kInvPhi;
				f1 = distanceAt(x1);
			} else {
				lo = x1;
				x1 = x2;
				f1 = f2;
				x2 = lo + (hi - lo) * kInvPhi;
				f2 = distanceAt(x2);
			}
		}
		return (std::min)({ f1, f2, distanceAt(0.0f), distanceAt(1.0f) });
	}

	bool Intersects(const OBB& obbA, const OBB& obbB) {
		auto projection = [](const OBB& obb, const Vector3& axis) {
			return std::abs(obb.size.x * Vector3::Dot(obb.axes[0], axis)) +
				std::abs(obb.size.y * Vector3::Dot(obb.axes[1], axis)) +
				std::abs(obb.size.z * Vector3::Dot(obb.axes[2], axis));
		};

		// 分離軸は 3 + 3 + 3x3 の15本
		std::array<Vector3, 15> axes;
		size_t axisCount = 0;
		for (const Vector3& axis : obbA.axes) {
			axes[axisCount++] = axis;
		}
		for (const Vector3& axis : obbB.axes) {
			axes[axisCount++] = axis;
		}
		for (const Vector3& axisA : obbA.axes) {
			for (const Vector3& axisB : obbB.axes) {
				axes[axisCount++] = Vector3::Cross(axisA, axisB);
			}
		}

		const Vector3 between = obbA.center - obbB.center;
		for (size_t i = 0; i < axisCount; ++i) {
			if (axes[i].Length() < std::numeric_limits<float>::epsilon()) {
				continue;
			}
			const Vector3 axis = axes[i].Normalize();
			if (std::abs(Vector3::Dot(between, axis)) > projection(obbA, axis) + projection(obbB, axis)) {
				return false;
			}
		}
		return true;
	}

	bool Intersects(const Sphere& a, const Sphere& b) {
		const Vector3 d = a.center - b.center;
		const float radius = a.radius + b.radius;
		return Vector3::Dot(d, d) <= radius * radius;
	}

	bool Intersects(const Sphere& sphere, const OBB& obb) {
		return LocalBoxDistanceSq(ToLocal(sphere.center, obb), obb.size) <= sphere.radius * sphere.radius;
	}

	bool Intersects(const Capsule& capsule, const Sphere& sphere) {
		const Vector3 d = ClosestPointOnSegment(sphere.center, capsule.a, capsule.b) - sphere.center;
		const float radius = capsule.radius + sphere.radius;
		return Vector3::Dot(d, d) <= radius * radius;
	}

	bool Intersects(const Capsule& a, const Capsule& b) {
		Vector3 onA;
		Vector3 onB;
		const float radius = a.radius + b.radius;
		return ClosestPointsSegmentSegment(a.a, a.b, b.a, b.b, onA, onB) <= radius * radius;
	}

	bool Intersects(const Capsule& capsule, const OBB& obb) {
		return SegmentOBBDistanceSq(capsule.a, capsule.b, obb) <= capsule.radius * capsule.radius;
	}

	bool Raycast(const Ray& ray, const Sphere& sphere, float maxDistance, RayHit& hit) {
		float t = 0.0f;
		if (!RaySphereSurface(ray.origin, ray.direction, sphere.center, sphere.radius, t) || t > maxDistance) {
			return false;
		}
		hit.distance = t;
		hit.point = ray.origin + ray.direction * t;
		hit.normal = (hit.point - sphere.center).Normalize();
		return true;
	}

	bool Raycast(const Ray& ray, const OBB& obb, float maxDistance, RayHit& hit) {
		const Vector3 origin = ToLocal(ray.origin, obb);
		const Vector3 direction = {
			Vector3::Dot(ray.direction, obb.axes[0]),
			Vector3::Dot(ray.direction, obb.axes[1]),
			Vector3::Dot(ray.direction, obb.axes[2]) };
		const float o[3] = { origin.x, origin.y, origin.z };
		const float d[3] = { direction.x, direction.y, direction.z };
		const float h[3] = { obb.size.x, obb.size.y, obb.size.z };
		if (std::abs(o[0]) <= h[0] && std::abs(o[1]) <= h[1] && std::abs(o[2]) <= h[2]) {
			return false;
		}

		// 3組の平面の間に入る区間を重ねる
		float tMin = 0.0f;
		float tMax = maxDistance;
		int entryAxis = -1;
		float entrySign = 0.0f;
		for (int i = 0; i < 3; i++) {
			if (std::abs(d[i]) < kEpsilon) {
				if (std::abs(o[i]) > h[i]) {
					return false;
				}
				continue;
			}
			float t1 = (-h[i] - o[i]) / d[i];
			float t2 = (h[i] - o[i]) / d[i];
			// 入る側の面の外向き
			float sign = -1.0f;
			if (t1 > t2) {
				std::swap(t1, t2);
				sign = 1.0f;
			}
			if (t1 > tMin) {
				tMin = t1;
				entryAxis = i;
				entrySign = sign;
			}
			tMax = (std::min)(tMax, t2);
			if (tMin > tMax) {
				return false;
			}
		}
		if (entryAxis < 0) {
			return false;
		}
		hit.distance = tMin;
		hit.point = ray.origin + ray.direction * tMin;
		hit.normal = obb.axes[entryAxis] * entrySign;
		return true;
	}

	// Inigo Quilez の capIntersect と同じ考え方。胴の円柱に当たらなければ近い方の端の球
	bool Raycast(const Ray& ray, const Capsule& capsule, float maxDistance, RayHit& hit) {
		const Vector3 nearest = ClosestPointOnSegment(ray.origin, capsule.a, capsule.b);
		if (Vector3::Dot(ray.origin - nearest, ray.origin - nearest) <= capsule.radius * capsule.radius) {
			return false;
		}

		const Vector3 ba = capsule.b - capsule.a;
		const Vector3 oa = ray.origin - capsule.a;
		const float baba = Vector3::Dot(ba, ba);
		const float bard = Vector3::Dot(ba, ray.direction);
		const float baoa = Vector3::Dot(ba, oa);
		const float rdoa = Vector3::Dot(ray.direction, oa);
		const float oaoa = Vector3::Dot(oa, oa);
		const float qa = baba - bard * bard;

		float t = -1.0f;
		// 胴 (軸と平行なレイや長さ0のカプセルは胴に当たらない)
		if (baba > kEpsilon && qa > kEpsilon * baba) {
			const float qb = baba * rdoa - baoa * bard;
			const float qc = baba * oaoa - baoa * baoa - capsule.radius * capsule.radius * baba;
			const float discriminant = qb * qb - qa * qc;
			if (discriminant >= 0.0f) {
				const float bodyT = (-qb - std::sqrt(discriminant)) / qa;
				const float y = baoa + bodyT * bard;
				if (y > 0.0f && y < baba && bodyT >= 0.0f) {
					t = bodyT;
				}
			}
		}
		// 両端の球
		if (t < 0.0f) {
			float capT = 0.0f;
			for (const Vector3& end : { capsule.a, capsule.b }) {
				if (RaySphereSurface(ray.origin, ray.direction, end, capsule.radius, capT) && (t < 0.0f || capT < t)) {
					t = capT;
				}
			}
		}
		if (t < 0.0f || t > maxDistance) {
			return false;
		}
		hit.distance = t;
		hit.point = ray.origin + ray.direction * t;
		hit.normal = (hit.point - ClosestPointOnSegment(hit.point, capsule.a, capsule.b)).Normalize();
		return true;
	}

}
