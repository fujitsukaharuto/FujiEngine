#pragma once
#define _USE_MATH_DEFINES
#include <vector>
#include <string>
#include <map>

#include "Engine/Math/Vector/Vector2.h"
#include "Engine/Math/Vector/Vector3.h"
#include "Engine/Math/Vector/Vector4.h"
#include "Engine/Math/Vector/Vector2Matrix.h"
#include "Engine/Math/Matrix/Matrix2x2.h"
#include "Engine/Math/Matrix/Matrix3x3.h"
#include "Engine/Math/Matrix/Matrix4x4.h"
#include "Engine/Math/Quaternion/Quaternion.h"

namespace Math {
	/// <summary>
	/// QuaternionベースのTransform
	/// </summary>
	struct QuaternionTrans {
		Vector3 scale = { 1.0f,1.0f,1.0f };
		Quaternion rotate = Quaternion();
		Vector3 translate = { 0.0f,0.0f,0.0f };
	};

	/// <summary>
	/// Transformのデータ
	/// </summary>
	struct Trans {
		Vector3 scale;
		Vector3 rotate; // Quaternion
		Vector3 translate;

		/// <summary>ペアレントの設定</summary>
		void SetParent(Trans* trans) { parent = trans; }
		/// <summary>スケールを適用しないペアレント</summary>
		void SetNoneScaleParent(bool is) { isNoneScaleParent = is; }
		/// <summary>カメラにペアレント</summary>
		void SetCameraParent(bool is) { isCameraParent = is; }

		//========================================================================*/
		//* Getter
		Matrix4x4 GetWorldMat() const;
		Matrix4x4 GetNoneScaleWorldMat() const;
		/// <summary>ペアレントを含めたワールド座標</summary>
		Vector3 GetWorldPos() const;
		Vector3 GetRotation();

		Trans* parent = nullptr;
		Matrix4x4* animeParent = nullptr;

		bool isNoneScaleParent = false;
		bool isCameraParent = false;
	};

	/// <summary>
	/// AABBのデータ
	/// </summary>
	struct AABB {
		Vector3 min;
		Vector3 max;
	};

	/// <summary>
	/// TransformMatrixのデータ
	/// </summary>
	struct TransformationMatrix {
		Matrix4x4 WVP;
		Matrix4x4 World;
		Matrix4x4 WorldInverseTransPose;
	};

	/// <summary>
	/// GPUに送るカメラ行列のデータ
	/// </summary>
	struct CameraForGPU {
		Vector3 worldPosition;
	};

	/// <summary>
	/// 球のデータ
	/// </summary>
	struct Sphere {
		Vector3 center;
		float radius;
	};

	/// <summary>
	/// アフィン行列
	/// </summary>
	Matrix3x3 MakeAffineMat(const Vector2& scale, float rotate, const Vector2& translate);

	/// <summary>
	/// 3x3行列の積
	/// </summary>
	Matrix3x3 Multiply(const Matrix3x3& matrix1, const Matrix3x3& matrix2);

	/// <summary>
	/// ベクトルと2x2行列の積
	/// </summary>
	Vector2 Multiply(const Vector2& vector, const Matrix2x2& matrix);

	/// <summary>
	/// 2x2行列の逆行列
	/// </summary>
	Matrix2x2 Inverse(const Matrix2x2& matrix);

	/// <summary>
	/// 3x3行列の逆行列
	/// </summary>
	Matrix3x3 Inverse(const Matrix3x3& matrix);

	/// <summary>
	/// 正射影行列
	/// </summary>
	Matrix3x3 MakeOrthographicMat(float left, float top, float right, float bottom);

	/// <summary>
	/// ビューポート行列
	/// </summary>
	Matrix3x3 MakeViewportMat(float left, float top, float width, float hight);

	/// <summary>
	/// レンダリング
	/// </summary>
	Matrix3x3 MakeWvpVpMat(const Matrix3x3& world, const Matrix3x3& view, const Matrix3x3& ortho, const Matrix3x3& viewPort);

	/// <summary>
	/// ワールド座標に変える
	/// </summary>
	Vector2 Transform(const Vector2& vector, const Matrix3x3& matrix);

	/// <summary>
	/// ベクトルを行列で変換し、ワールド座標に変換する。
	/// </summary>
	Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix);

	/// <summary>
	/// ベクトルの方向成分のみを行列で変換する（位置変換を無視）。
	/// </summary>
	Vector3 TransformDirection(const Vector3& vector, const Matrix4x4& matrix);

	/// <summary>
	/// 2つの4x4行列を掛け合わせる。
	/// </summary>
	Matrix4x4 Multiply(const Matrix4x4& matrix1, const Matrix4x4& matrix2);

	/// <summary>
	/// 4x4行列の転置を返す。
	/// </summary>
	Matrix4x4 Transpose(const Matrix4x4& m);

	/// <summary>
	/// 4x4行列の逆行列を求める。
	/// </summary>
	Matrix4x4 Inverse(const Matrix4x4& matrix);

	/// <summary>行列からスケール成分を除去した行列を返す</summary>
	Matrix4x4 RemoveScale(const Matrix4x4& m);

	/// <summary>
	/// 単位行列を生成する。
	/// </summary>
	Matrix4x4 MakeIdentity4x4();

	/// <summary>
	/// 平行移動行列を生成する。
	/// </summary>
	Matrix4x4 MakeTranslateMatrix(const Vector3& translate);

	/// <summary>
	/// 拡縮行列を生成する。
	/// </summary>
	Matrix4x4 MakeScaleMatrix(const Vector3& scale);

	/// <summary>
	/// X軸回転行列を生成する。
	/// </summary>
	Matrix4x4 MakeRotateXMatrix(float rad);

	/// <summary>
	/// Y軸回転行列を生成する。
	/// </summary>
	Matrix4x4 MakeRotateYMatrix(float rad);

	/// <summary>
	/// Z軸回転行列を生成する。
	/// </summary>
	Matrix4x4 MakeRotateZMatrix(float rad);

	/// <summary>
	/// XYZ順で回転する回転行列を生成する。
	/// </summary>
	Matrix4x4 MakeRotateXYZMatrix(const Vector3& rota);

	/// <summary>
	/// 拡縮・回転・平行移動を組み合わせたアフィン変換行列を生成する。
	/// </summary>
	Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);

	/// <summary>
	/// クォータニオン回転を使用したアフィン変換行列を生成する。
	/// </summary>
	Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Quaternion& rotate, const Vector3& translate);

	/// <summary>
	/// 透視投影行列を生成する。
	/// </summary>
	Matrix4x4 MakePerspectiveFovMatrix(float fovy, float aspectRation, float nearClip, float farClip);

	/// <summary>
	/// 正射影行列を生成する。
	/// </summary>
	Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);

	/// <summary>
	/// ビューポート行列を生成する。
	/// </summary>
	Matrix4x4 MakeViewportMat(float left, float top, float width, float height, float minDepth, float maxDepth);

	/// <summary>
	/// 法線ベクトルを行列で変換する（平行移動成分を無視）。
	/// </summary>
	Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m);

	/// <summary>
	/// 2つの値の線形補間を行う。
	/// </summary>
	float Lerp(float v1, float v2, float t);

	/// <summary>
	/// 2つのベクトルの線形補間を行う。
	/// </summary>
	Vector3 Lerp(const Vector3& v1, const Vector3& v2, float t);

	/// <summary>
	/// 2つのベクトルの外積を計算する。
	/// </summary>
	Vector3 Cross(const Vector3& a, const Vector3& b);

	/// <summary>
	/// 値を指定範囲内にクランプする。
	/// </summary>
	float Clamp(float x, float min, float max);

	/// <summary>
	/// Catmull-Romスプライン補間による1点の計算を行う。
	/// </summary>
	Vector3 CatmullRomPoint(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t);

	/// <summary>
	/// 複数の制御点からCatmull-Romスプライン曲線上の点を求める。
	/// </summary>
	Vector3 CatmullRom(const std::vector<Vector3>& control, float t);

	/// <summary>
	/// 回転行列からオイラー角を抽出する。
	/// </summary>
	Vector3 ExtractEulerAngles(const Matrix4x4& rotationMatrix);

	/// <summary>
	/// 角度の短い方を補間する線形補間（Lerp）を行う。
	/// </summary>
	float LerpShortAngle(float a, float b, float t);

	/// <summary>
	/// 前方ベクトルと上方向ベクトルからビュー行列（LookAt行列）を生成する。
	/// </summary>
	Matrix4x4 MakeLookAtMatrix(const Vector3& forward, const Vector3& up);

	/// <summary>
	/// 任意軸まわりの回転行列を生成する。
	/// </summary>
	Matrix4x4 MakeRotationAxisAngle(const Vector3& axis, float angle);

	/// <summary>
	/// 点がAABB（軸平行境界ボックス）内に存在するか判定する。
	/// </summary>
	bool IsCollision(const AABB& aabb, const Vector3& point);

	/// <summary>
	/// 任意軸まわりの回転行列を生成する（同名別実装）。
	/// </summary>
	Matrix4x4 MakeRotateAxisAngle(const Vector3& axis, float angle);

	/// <summary>
	/// ある方向ベクトルを別の方向へ回転させる行列を生成する。
	/// </summary>
	Matrix4x4 DirectionToDirection(const Vector3& from, const Vector3& to);

	/// <summary>
	/// 行列の値を float配列に変換する。
	/// </summary>
	void ToFloatArray(const Matrix4x4& m, float out[16]);

	/// <summary>
	/// float配列からMatrix4x4を構築する。
	/// </summary>
	Matrix4x4 FromFloatArray(const float in[16]);
}