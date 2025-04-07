#pragma once
#include <cfloat>

#include "Vector3.h"
#include "Vector2Matrix.h"

class Quaternion {
public:

	float x;
	float y;
	float z;
	float w;

	Quaternion() : x(0), y(0), z(0), w(1) {}
	Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

	~Quaternion() = default;

	// 単位クォータニオン
	static Quaternion IdentityQuaternion() {
		return Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
	}

	// 軸と角度(ラジアン)からクォータニオン
	static Quaternion AngleAxis(float angle, const Vector3& axis) {
		float halfAngle = angle * 0.5f;
		float sinHalfAngle = std::sin(halfAngle);
		float cosHalfAngle = std::cos(halfAngle);
		Vector3 normAxis = axis.Normalize();
		return Quaternion(
			normAxis.x * sinHalfAngle,
			normAxis.y * sinHalfAngle,
			normAxis.z * sinHalfAngle,
			cosHalfAngle
		);
	}

	// クォータニオンの乗算
	static Quaternion Multiply(const Quaternion& lhs, const Quaternion& rhs) {
		return Quaternion(
			lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y, // x
			lhs.w * rhs.y - lhs.x * rhs.z + lhs.y * rhs.w + lhs.z * rhs.x, // y
			lhs.w * rhs.z + lhs.x * rhs.y - lhs.y * rhs.x + lhs.z * rhs.w, // z
			lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z  // w
		);
	}

	static Vector3 RotateVector(const Vector3& vector, const Quaternion& quaternion) {
		Quaternion vecQ(vector.x, vector.y, vector.z, 0.0f);
		Quaternion invQ = quaternion.Inverse();
		Quaternion result = quaternion * vecQ * invQ;
		return Vector3(result.x, result.y, result.z);
	}

	static float Dot(const Quaternion& q1, const Quaternion& q2) {
		return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
	}

	float Norm() const {
		return std::sqrtf(x * x + y * y + z * z + w * w);
	}

	Quaternion Normalize() const {
		float norm = Norm();
		if (norm > 0.0f) {
			float invNorm = 1.0f / norm;
			return Quaternion(x * invNorm, y * invNorm, z * invNorm, w * invNorm);
		}
		return IdentityQuaternion();
	}

	Quaternion Conjugate() const {
		return Quaternion(-x, -y, -z, w);
	}

	Quaternion Inverse() const {
		float normSquared = x * x + y * y + z * z + w * w;
		if (normSquared > 0.0f) {
			float invNormSquared = 1.0f / normSquared;
			return Quaternion(-x * invNormSquared, -y * invNormSquared, -z * invNormSquared, w * invNormSquared);
		}
		return IdentityQuaternion();
	}

	static Quaternion Slerp(const Quaternion& q0, const Quaternion& q1, float t) {
		Quaternion q0_0 = q0;
		// q0とq1の内積
		float dot = Dot(q0_0, q1);

		// 内積が負の場合、もう片方の回転を利用する
		if (dot < 0.0f) {

			q0_0 = -q0_0;
			dot = -dot;
		}

		if (dot >= 1.0f - FLT_EPSILON) {

			return (1.0f - t) * q0_0 + t * q1;
		}

		// なす角を求める
		float theta = std::acos(dot);
		float sinTheta = std::sin(theta);

		// 補完係数を計算
		float scale0 = std::sin((1.0f - t) * theta) / sinTheta;
		float scale1 = std::sin(t * theta) / sinTheta;

		// 補完後のクォータニオンを求める
		return q0_0 * scale0 + q1 * scale1;
	}

	Matrix4x4 MakeRotateMatrix()const {

		Matrix4x4 result;
		float xx = x * x;
		float yy = y * y;
		float zz = z * z;
		float ww = w * w;
		float xy = x * y;
		float xz = x * z;
		float yz = y * z;
		float wx = w * x;
		float wy = w * y;
		float wz = w * z;

		result.m[0][0] = ww + xx - yy - zz;
		result.m[0][1] = 2.0f * (xy + wz);
		result.m[0][2] = 2.0f * (xz - wy);
		result.m[0][3] = 0.0f;

		result.m[1][0] = 2.0f * (xy - wz);
		result.m[1][1] = ww - xx + yy - zz;
		result.m[1][2] = 2.0f * (yz + wx);
		result.m[1][3] = 0.0f;

		result.m[2][0] = 2.0f * (xz + wy);
		result.m[2][1] = 2.0f * (yz - wx);
		result.m[2][2] = ww - xx - yy + zz;
		result.m[2][3] = 0.0f;

		result.m[3][0] = 0.0f;
		result.m[3][1] = 0.0f;
		result.m[3][2] = 0.0f;
		result.m[3][3] = 1.0f;

		return result;
	}


	Quaternion operator-() const {
		return Quaternion(-x, -y, -z, -w);
	}

	Quaternion operator*(const Quaternion& rhs) const {
		return Quaternion(
			w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y, // x
			w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x, // y
			w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w, // z
			w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z  // w
		);
	}

	Quaternion operator*(float scalar) const {
		return Quaternion(x * scalar, y * scalar, z * scalar, w * scalar);
	}

	// Quaternion と float の乗算（左側・非メンバ関数）
	friend Quaternion operator*(float scalar, const Quaternion& q) {
		return Quaternion(q.x * scalar, q.y * scalar, q.z * scalar, q.w * scalar);
	}

	// Quaternion の加算
	Quaternion operator+(const Quaternion& rhs) const {
		return Quaternion(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
	}

};
