#pragma once
#include <cfloat>
#include <numbers>

#include "Math/Vector/Vector3.h"
#include "Math/Vector/Vector4.h"
#include "Math/Matrix/Matrix4x4.h"

namespace Math {
	/// <summary>
	/// Quaternionクラス
	/// </summary>
	class Quaternion {
	public:

		float x;
		float y;
		float z;
		float w;

		Quaternion() : x(0), y(0), z(0), w(1) {}
		Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

		~Quaternion() = default;

		/// <summary>単位クォータニオン</summary>
		static Quaternion IdentityQuaternion() {
			return Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
		}

		/// <summary>軸と角度(ラジアン)からクォータニオン</summary>
		static Quaternion AngleAxis(float angle, const Vector3& axis);

		/// <summary>クォータニオンの乗算</summary>
		static Quaternion Multiply(const Quaternion& lhs, const Quaternion& rhs);

		/// <summary>Vectorをクォータニオンで回転</summary>
		static Vector3 RotateVector(const Vector3& vector, const Quaternion& quaternion);

		/// <summary>2つのクォータニオンの内積を求める</summary>
		static float Dot(const Quaternion& q1, const Quaternion& q2);

		/// <summary>クォータニオンのノルム（長さ）を求める</summary>
		float Norm() const {
			return std::sqrtf(x * x + y * y + z * z + w * w);
		}

		/// <summary>クォータニオンを正規化する</summary>
		Quaternion Normalize() const;

		/// <summary>クォータニオンの共役を求める</summary>
		Quaternion Conjugate() const {
			return Quaternion(-x, -y, -z, w);
		}

		/// <summary>クォータニオンの逆を求める</summary>
		Quaternion Inverse() const;

		/// <summary>2つのクォータニオン間を球面線形補間（SLerp）する</summary>
		static Quaternion SLerp(const Quaternion& q0, const Quaternion& q1, float t);

		/// <summary>クォータニオンから回転行列を生成する</summary>
		Matrix4x4 MakeRotateMatrix() const;

		/// <summary>回転行列をクォータニオンに変換する</summary>
		static Quaternion MatrixToQuaternion(const Matrix4x4& m);

		/// <summary>前方ベクトルと上方向ベクトルから回転クォータニオンを作成する</summary>
		static Quaternion LookRotation(const Vector3& forward, const Vector3& up = Vector3(0, 1, 0));

		/// <summary>オイラー角からクォータニオンを生成する</summary>
		static Quaternion FromEuler(const Vector3& euler);

		/// <summary>クォータニオンをオイラー角に変換する</summary>
		static Vector3 QuaternionToEuler(const Quaternion& q);

		/// <summary>ある方向ベクトルから別の方向ベクトルへの回転を表すクォータニオンを求める</summary>
		Quaternion DirectionToDirection(const Vector3& from, const Vector3& to);

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
}