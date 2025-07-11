#pragma once
#define _USE_MATH_DEFINES
#include <vector>
#include <string>
#include <map>

#include "Vector/Vector2.h"
#include "Vector/Vector3.h"
#include "Vector/Vector4.h"
#include "Vector/Vector2Matrix.h"
#include "Matrix/Matrix2x2.h"
#include "Matrix/Matrix3x3.h"
#include "Matrix/Matrix4x4.h"
#include "Quaternion/Quaternion.h"


struct QuaternioonTrans {
	Vector3 scale = { 1.0f,1.0f,1.0f };
	Quaternion rotate = Quaternion();
	Vector3 translate = { 0.0f,0.0f,0.0f };
};

struct Trans {
	Vector3 scale;
	Vector3 rotate; // Quater
	Vector3 translate;

	void SetNoneScaleParent(bool is) { isNoneScaleParent = is; }
	void SetCameraParent(bool is) { isCameraParent = is; }
	Matrix4x4 GetWorldMat() const;
	Matrix4x4 GetNoneScaleWorldMat() const;
	Vector3 GetRotation();
	Trans* parent = nullptr;
	Matrix4x4* animParent = nullptr;

	bool isNoneScaleParent = false;
	bool isCameraParent = false;
};

struct AABB {
	Vector3 min;
	Vector3 max;
};

struct TransformationMatrix {
	Matrix4x4 WVP;
	Matrix4x4 World;
	Matrix4x4 WorldInverseTransPose;
};

struct CameraForGPU {
	Vector3 worldPosition;
};

struct Sphere {
	Vector3 center;
	float radius;
};

/// <summary>
/// アフィン行列
/// </summary>
Matrix3x3 MakeAffineMat(Vector2 scale, float rotate, Vector2 translate);

/// <summary>
/// 3x3行列の積
/// </summary>
Matrix3x3 Multiply(Matrix3x3 matrix1, Matrix3x3 matrix2);

/// <summary>
/// ベクトルと2x2行列の積
/// </summary>
Vector2 Multiply(Vector2 vector, Matrix2x2 matrix);

/// <summary>
/// 2x2行列の逆行列
/// </summary>
Matrix2x2 Inverse(Matrix2x2 matrix);

/// <summary>
/// 3x3行列の逆行列
/// </summary>
Matrix3x3 Inverse(Matrix3x3 matrix);

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

Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix);

Matrix4x4 Multiply(const Matrix4x4& matrix1, const Matrix4x4& matrix2);

Matrix4x4 Transpose(const Matrix4x4& m);

Matrix4x4 Inverse(const Matrix4x4& matrix);

Matrix4x4 MakeIdentity4x4();

Matrix4x4 MakeTranslateMatrix(const Vector3& translate);

Matrix4x4 MakeScaleMatrix(const Vector3& scale);

Matrix4x4 MakeRotateXMatrix(float rad);

Matrix4x4 MakeRotateYMatrix(float rad);

Matrix4x4 MakeRotateZMatrix(float rad);

Matrix4x4 MakeRotateXYZMatrix(const Vector3& rota);

Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);

Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Quaternion& rotate, const Vector3& translate);

Matrix4x4 MakePerspectiveFovMatrix(float fovy, float aspectRation, float nearClip, float farClip);

Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);

Matrix4x4 MakeViewportMat(float left, float top, float width, float height, float minDepth, float maxDepth);

Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m);

float Lerp(float v1, float v2, float t);

Vector3 Lerp(const Vector3& v1, const Vector3& v2, float t);

Vector3 Cross(const Vector3& a, const Vector3& b);

float Clamp(float x, float min, float max);

Vector3 CatmullRomPoint(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t);

Vector3 CatmullRom(const std::vector<Vector3>& control, float t);

Vector3 ExtractEulerAngles(const Matrix4x4& rotationMatrix);

float LerpShortAngle(float a, float b, float t);

Matrix4x4 MakeLookAtMatrix(const Vector3& forward, const Vector3& up);

Matrix4x4 MakeRotationAxisAngle(const Vector3& axis, float angle);

bool IsCollision(const AABB& aabb, const Vector3& point);

Matrix4x4 MakeRotateAxisAngle(const Vector3& axis, float angle);

Matrix4x4 DirectionToDirection(const Vector3& from, const Vector3& to);

void ToFloatArray(const Matrix4x4& m, float out[16]);

Matrix4x4 FromFloatArray(const float in[16]);
