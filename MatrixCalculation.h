#pragma once
#define _USE_MATH_DEFINES
#include <cmath>
#include <assert.h>

#include "Vector2.h"
#include "Vector3.h"
#include "Vector2Matrix.h"


/// <summary>
/// 2次元ベクトルのスカラー倍を求める
/// </summary>
Vector2 Multiply(const Vector2& vec, const float& num);

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

Matrix4x4 Multiply(Matrix4x4& matrix1, Matrix4x4& matrix2);

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

Matrix4x4 MakePerspectiveFovMatrix(float fovy, float aspectRation, float nearClip, float farClip);

Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);

Matrix4x4 MakeViewportMat(float left, float top, float width, float height, float minDepth, float maxDepth);
