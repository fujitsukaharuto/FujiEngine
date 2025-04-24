#pragma once
#include <cfloat>
#include <numbers>

#include "Math/Vector/Vector3.h"
#include "Math/Vector/Vector4.h"
#include "Math/Matrix/Matrix4x4.h"

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

			return ((1.0f - t) * q0_0 + t * q1).Normalize();
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

	Matrix4x4 MakeRotateMatrix() const {

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

	static Quaternion MatrixToQuaternion(const Matrix4x4& m) {
		float trace = m.m[0][0] + m.m[1][1] + m.m[2][2];
		Quaternion q;

		if (trace > 0.0f) {
			float s = std::sqrt(trace + 1.0f) * 2.0f; // s = 4 * qw
			q.w = 0.25f * s;
			q.x = (m.m[2][1] - m.m[1][2]) / s;
			q.y = (m.m[0][2] - m.m[2][0]) / s;
			q.z = (m.m[1][0] - m.m[0][1]) / s;
		} else if (m.m[0][0] > m.m[1][1] && m.m[0][0] > m.m[2][2]) {
			float s = std::sqrt(1.0f + m.m[0][0] - m.m[1][1] - m.m[2][2]) * 2.0f; // s = 4 * qx
			q.w = (m.m[2][1] - m.m[1][2]) / s;
			q.x = 0.25f * s;
			q.y = (m.m[0][1] + m.m[1][0]) / s;
			q.z = (m.m[0][2] + m.m[2][0]) / s;
		} else if (m.m[1][1] > m.m[2][2]) {
			float s = std::sqrt(1.0f + m.m[1][1] - m.m[0][0] - m.m[2][2]) * 2.0f; // s = 4 * qy
			q.w = (m.m[0][2] - m.m[2][0]) / s;
			q.x = (m.m[0][1] + m.m[1][0]) / s;
			q.y = 0.25f * s;
			q.z = (m.m[1][2] + m.m[2][1]) / s;
		} else {
			float s = std::sqrt(1.0f + m.m[2][2] - m.m[0][0] - m.m[1][1]) * 2.0f; // s = 4 * qz
			q.w = (m.m[1][0] - m.m[0][1]) / s;
			q.x = (m.m[0][2] + m.m[2][0]) / s;
			q.y = (m.m[1][2] + m.m[2][1]) / s;
			q.z = 0.25f * s;
		}

		return q.Normalize(); // 念のため正規化
	}

	static Quaternion LookRotation(const Vector3& forward, const Vector3& up = Vector3(0, 1, 0)) {
		Vector3 z = forward.Normalize();
		Vector3 x = up.Cross(z).Normalize();
		Vector3 y = z.Cross(x);

		Matrix4x4 m;
		m.m[0][0] = x.x; m.m[0][1] = y.x; m.m[0][2] = z.x; m.m[0][3] = 0;
		m.m[1][0] = x.y; m.m[1][1] = y.y; m.m[1][2] = z.y; m.m[1][3] = 0;
		m.m[2][0] = x.z; m.m[2][1] = y.z; m.m[2][2] = z.z; m.m[2][3] = 0;
		m.m[3][0] = 0;   m.m[3][1] = 0;   m.m[3][2] = 0;   m.m[3][3] = 1;

		// 変換行列からクォータニオンを作る（関数が必要）
		return MatrixToQuaternion(m);
	}

	static Quaternion FromEuler(const Vector3& euler) {
		float pitch = euler.x; // X軸（上下）
		float yaw = euler.y; // Y軸（左右）
		float roll = euler.z; // Z軸（ひねり）

		// 必要に応じて順序を変更
		Quaternion qx = Quaternion::AngleAxis(pitch, Vector3(1, 0, 0));
		Quaternion qy = Quaternion::AngleAxis(yaw, Vector3(0, 1, 0));
		Quaternion qz = Quaternion::AngleAxis(roll, Vector3(0, 0, 1));

		return qy * qx * qz; // 例：Yaw → Pitch → Roll
	}

	static Vector3 QuaternionToEuler(const Quaternion& q) {
		Vector3 euler;

		// roll (x-axis rotation)
		float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
		float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
		euler.x = std::atan2(sinr_cosp, cosr_cosp);

		// pitch (y-axis rotation)
		float sinp = 2.0f * (q.w * q.y - q.z * q.x);
		if (std::abs(sinp) >= 1.0f)
			euler.y = std::copysign(std::numbers::pi_v<float> / 2.0f, sinp); // use 90 degrees if out of range
		else
			euler.y = std::asin(sinp);

		// yaw (z-axis rotation)
		float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
		float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
		euler.z = std::atan2(siny_cosp, cosy_cosp);

		return euler;
	}

	Quaternion DirectionToDirection(const Vector3& from, const Vector3& to) {
		Vector3 f = from.Normalize();
		Vector3 t = to.Normalize();

		float dot = Vector3::Dot(f, t);

		if (dot >= 1.0f - FLT_EPSILON) {
			// 完全に同じ方向なら回転不要
			return Quaternion::IdentityQuaternion();
		} else if (dot <= -1.0f + FLT_EPSILON) {
			// 真逆方向の場合、回転軸を見つけて180度回す必要あり
			Vector3 orthogonal = Vector3(1, 0, 0).Cross(f);
			if (orthogonal.Length() < FLT_EPSILON) {
				orthogonal = Vector3(0, 1, 0).Cross(f);
			}
			orthogonal = orthogonal.Normalize();
			return Quaternion::AngleAxis(std::numbers::pi_v<float>, orthogonal);
		} else {
			Vector3 axis = f.Cross(t).Normalize();
			float angle = std::acos(dot);
			return Quaternion::AngleAxis(angle, axis);
		}
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
