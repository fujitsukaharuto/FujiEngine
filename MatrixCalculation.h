#pragma once
#define _USE_MATH_DEFINES
#include <cmath>
#include <assert.h>

#include "MyVec2.h"
#include "MyVec3.h"
#include "Vector2Matrix.h"


/// <summary>
/// 2次元ベクトルのスカラー倍を求める
/// </summary>
MyVec2 Multiply(const MyVec2& vec, const float& num);

/// <summary>
/// アフィン行列
/// </summary>
Matrix3x3 MakeAffineMat(MyVec2 scale, float rotate, MyVec2 translate);

/// <summary>
/// 3x3行列の積
/// </summary>
Matrix3x3 Multiply(Matrix3x3 matrix1, Matrix3x3 matrix2);

/// <summary>
/// ベクトルと2x2行列の積
/// </summary>
MyVec2 Multiply(MyVec2 vector, Matrix2x2 matrix);

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
MyVec2 Transform(const MyVec2& vector, const Matrix3x3& matrix);

MyVec3 Transform(const MyVec3& vector, const Matrix4x4& matrix);

Matrix4x4 Multiply(Matrix4x4& matrix1, Matrix4x4& matrix2);

Matrix4x4 Transpose(const Matrix4x4& m);

Matrix4x4 Inverse(const Matrix4x4& matrix);

Matrix4x4 MakeIdentity4x4();

Matrix4x4 MakeTranslateMatrix(const MyVec3& translate);

Matrix4x4 MakeScaleMatrix(const MyVec3& scale);

Matrix4x4 MakeRotateXMatrix(float rad);

Matrix4x4 MakeRotateYMatrix(float rad);

Matrix4x4 MakeRotateZMatrix(float rad);

Matrix4x4 MakeRotateXYZMatrix(const MyVec3& rota);

Matrix4x4 MakeAffineMatrix(const MyVec3& scale, const MyVec3& rotate, const MyVec3& translate);

Matrix4x4 MakePerspectiveFovMatrix(float fovy, float aspectRation, float nearClip, float farClip);

Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);

Matrix4x4 MakeViewportMat(float left, float top, float width, float height, float minDepth, float maxDepth);
