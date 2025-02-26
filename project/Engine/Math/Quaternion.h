#pragma once
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
		float dot = Dot(q0, q1);

		bool flip = false;

		if (dot < 0.0f) {
			flip = true;
			dot = -dot;
		}

		float s0, s1;
		const float slerpEpsilon = 1e-6f;

		if (dot > (1.0f - slerpEpsilon)) {

			s0 = 1.0f - t;
			s1 = (flip) ? -t : t;
		} else {
			float omega = std::acos(dot);
			float invSinOmega = 1 / std::sin(omega);

			s0 = std::sin((1.0f - t) * omega) * invSinOmega;
			s1 = (flip)
				? -std::sin(t * omega) * invSinOmega
				: std::sin(t * omega) * invSinOmega;
		}

		return Quaternion(
			s0 * q0.x + s1 * q1.x,
			s0 * q0.y + s1 * q1.y,
			s0 * q0.z + s1 * q1.z,
			s0 * q0.w + s1 * q1.w
		);
	}

	Matrix4x4 MakeRotateMatrix()const {
		Matrix4x4 mat;

		// クォータニオンを使って回転行列を計算
		mat.m[0][0] = 1.0f - 2.0f * (y * y + z * z);
		mat.m[0][1] = 2.0f * (x * y + w * z);
		mat.m[0][2] = 2.0f * (x * z - w * y);
		mat.m[0][3] = 0;

		mat.m[1][0] = 2.0f * (x * y - w * z);
		mat.m[1][1] = 1.0f - 2.0f * (x * x + z * z);
		mat.m[1][2] = 2.0f * (y * z + w * x);
		mat.m[1][3] = 0;

		mat.m[2][0] = 2.0f * (x * z + w * y);
		mat.m[2][1] = 2.0f * (y * z - w * x);
		mat.m[2][2] = 1.0f - 2.0f * (x * x + y * y);
		mat.m[2][3] = 0;

		mat.m[3][0] = 0;
		mat.m[3][1] = 0;
		mat.m[3][2] = 0;
		mat.m[3][3] = 1;

		return mat;
	}




	Quaternion operator*(const Quaternion& rhs) const {
		return Quaternion(
			w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y, // x
			w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x, // y
			w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w, // z
			w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z  // w
		);
	}

};
