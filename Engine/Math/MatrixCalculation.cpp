#include "MatrixCalculation.h"
#include <algorithm>

Vector2 Multiply(const Vector2& vec, const float& num)
{
	return { vec.x * num,vec.y * num };
}

Matrix3x3 MakeAffineMat(Vector2 scale, float rotate, Vector2 translate) {
	Matrix3x3 result{};
	for (int y = 0; y < 2; y++)
	{
		for (int x = 0; x < 3; x++)
		{
			result.m[y][x] = 0;
		}
	}

	result.m[0][0] = scale.x * cosf(rotate); result.m[0][1] = scale.x * sinf(rotate);
	result.m[1][0] = scale.y * -sinf(rotate); result.m[1][1] = scale.y * cosf(rotate);
	result.m[2][0] = translate.x; result.m[2][1] = translate.y; result.m[2][2] = 1;

	return result;
}

Matrix3x3 Multiply(Matrix3x3 matrix1, Matrix3x3 matrix2) {
	Matrix3x3 result{};

	for (int row = 0; row < 3; row++) {
		for (int column = 0; column < 3; column++) {
			for (int i = 0; i < 3; i++) {
				result.m[row][column] += matrix1.m[row][i] * matrix2.m[i][column];
			}
		}
	}

	return result;
}

Vector2 Multiply(Vector2 vector, Matrix2x2 matrix) {
	Vector2 result{};

	result.x = vector.x * matrix.m[0][0] + vector.y * matrix.m[1][0];
	result.y = vector.x * matrix.m[0][1] + vector.y * matrix.m[1][1];

	return result;
}

Matrix2x2 Inverse(Matrix2x2 matrix)
{
	Matrix2x2 result{};
	Matrix2x2 m{};
	m.m[0][0] = matrix.m[1][1]; m.m[0][1] = -(matrix.m[0][1]);
	m.m[1][0] = -(matrix.m[1][0]); m.m[1][1] = matrix.m[0][0];

	float number;
	number = (matrix.m[0][0] * matrix.m[1][1] - matrix.m[0][1] * matrix.m[1][0]);
	if (number != 0)
	{
		number = 1.0f / number;
		for (int i = 0; i < 2; i++)
		{
			for (int j = 0; j < 2; j++)
			{
				result.m[j][i] = number * m.m[j][i];
			}
		}
	}

	return result;
}

Matrix3x3 Inverse(Matrix3x3 matrix)
{
	Matrix3x3 m{};
	m.m[0][0] = matrix.m[1][1] * matrix.m[2][2] - matrix.m[1][2] * matrix.m[2][1];
	m.m[0][1] = -(matrix.m[0][1] * matrix.m[2][2] - matrix.m[0][2] * matrix.m[2][1]);
	m.m[0][2] = matrix.m[0][1] * matrix.m[1][2] - matrix.m[0][2] * matrix.m[1][1];

	m.m[1][0] = -(matrix.m[1][0] * matrix.m[2][2] - matrix.m[1][2] * matrix.m[2][0]);
	m.m[1][1] = matrix.m[0][0] * matrix.m[2][2] - matrix.m[0][2] * matrix.m[2][0];
	m.m[1][2] = -(matrix.m[0][0] * matrix.m[1][2] - matrix.m[0][2] * matrix.m[1][0]);

	m.m[2][0] = matrix.m[1][0] * matrix.m[2][1] - matrix.m[1][1] * matrix.m[2][0];
	m.m[2][1] = -(matrix.m[0][0] * matrix.m[2][1] - matrix.m[0][1] * matrix.m[2][0]);
	m.m[2][2] = matrix.m[0][0] * matrix.m[1][1] - matrix.m[0][1] * matrix.m[1][0];

	float number;
	number = matrix.m[0][0] * matrix.m[1][1] * matrix.m[2][2] + matrix.m[0][1] * matrix.m[1][2] * matrix.m[2][0] + matrix.m[0][2] * matrix.m[1][0] * matrix.m[2][1] - (matrix.m[0][2] * matrix.m[1][1] * matrix.m[2][0]) - (matrix.m[0][1] * matrix.m[1][0] * matrix.m[2][2]) - (matrix.m[0][0] * matrix.m[1][2] * matrix.m[2][1]);
	number = 1.0f / number;

	Matrix3x3 result{};

	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			result.m[j][i] = number * m.m[j][i];
		}
	}

	return result;
}

Matrix3x3 MakeOrthographicMat(float left, float top, float right, float bottom) {
	Matrix3x3 result{};

	result.m[0][0] = 2.0f / (right - left);
	result.m[1][1] = 2.0f / (top - bottom);
	result.m[2][0] = (left + right) / (left - right);
	result.m[2][1] = (top + bottom) / (bottom - top);
	result.m[2][2] = 1.0f;

	return result;
}

Matrix3x3 MakeViewportMat(float left, float top, float width, float hight) {

	Matrix3x3 result{};

	result.m[0][0] = width / 2;
	result.m[1][1] = -(hight / 2);
	result.m[2][0] = left + width / 2;
	result.m[2][1] = top + hight / 2;
	result.m[2][2] = 1;

	return result;

}

Matrix3x3 MakeWvpVpMat(const Matrix3x3& world, const Matrix3x3& view, const Matrix3x3& ortho, const Matrix3x3& viewPort) {
	Matrix3x3 result{};

	result = Multiply(world, view);
	result = Multiply(result, ortho);
	result = Multiply(result, viewPort);

	return result;
}

Vector2 Transform(const Vector2& vector, const Matrix3x3& matrix) {

	Vector2 result{};
	result.x = vector.x * matrix.m[0][0] + vector.y * matrix.m[1][0] + matrix.m[2][0];
	result.y = vector.x * matrix.m[0][1] + vector.y * matrix.m[1][1] + matrix.m[2][1];
	float w = vector.x * matrix.m[0][2] + vector.y * matrix.m[1][2] + matrix.m[2][2];
	assert(w != 0.0f);
	result.x /= w;
	result.y /= w;

	return result;
}

Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix)
{
	Vector3 result{};
	result.x = vector.x * matrix.m[0][0] + vector.y * matrix.m[1][0] + vector.z * matrix.m[2][0] + matrix.m[3][0];
	result.y = vector.x * matrix.m[0][1] + vector.y * matrix.m[1][1] + vector.z * matrix.m[2][1] + matrix.m[3][1];
	result.z = vector.x * matrix.m[0][2] + vector.y * matrix.m[1][2] + vector.z * matrix.m[2][2] + matrix.m[3][2];
	float w = vector.x * matrix.m[0][3] + vector.y * matrix.m[1][3] + vector.z * matrix.m[2][3] + matrix.m[3][3];
	assert(w != 0.0f);
	result.x /= w;
	result.y /= w;
	result.z /= w;

	return result;
}

Matrix4x4 Multiply(const Matrix4x4& matrix1, const Matrix4x4& matrix2)
{
	Matrix4x4 result{};

	for (int row = 0; row < 4; row++) {
		for (int column = 0; column < 4; column++) {
			for (int i = 0; i < 4; i++) {
				result.m[row][column] += matrix1.m[row][i] * matrix2.m[i][column];
			}
		}
	}

	return result;
}

Matrix4x4 Transpose(const Matrix4x4& m)
{
	Matrix4x4 result{};

	for (int row = 0; row < 4; row++)
	{
		for (int col = 0; col < 4; col++)
		{
			result.m[row][col] = m.m[col][row];
		}
	}

	return result;
}

Matrix4x4 Inverse(const Matrix4x4& matrix)
{
	Matrix4x4 m{};
	m.m[0][0] = matrix.m[1][1] * matrix.m[2][2] * matrix.m[3][3] + matrix.m[1][2] * matrix.m[2][3] * matrix.m[3][1] + matrix.m[1][3] * matrix.m[2][1] * matrix.m[3][2]
		- matrix.m[1][3] * matrix.m[2][2] * matrix.m[3][1] - matrix.m[1][2] * matrix.m[2][1] * matrix.m[3][3] - matrix.m[1][1] * matrix.m[2][3] * matrix.m[3][2];
	
	m.m[0][1] = -(matrix.m[0][1] * matrix.m[2][2] * matrix.m[3][3]) - (matrix.m[0][2] * matrix.m[2][3] * matrix.m[3][1]) - (matrix.m[0][3] * matrix.m[2][1] * matrix.m[3][2])
		+ matrix.m[0][3] * matrix.m[2][2] * matrix.m[3][1] + matrix.m[0][2] * matrix.m[2][1] * matrix.m[3][3] + matrix.m[0][1] * matrix.m[2][3] * matrix.m[3][2];
	
	m.m[0][2] = matrix.m[0][1] * matrix.m[1][2] * matrix.m[3][3] + matrix.m[0][2] * matrix.m[1][3] * matrix.m[3][1] + matrix.m[0][3] * matrix.m[1][1] * matrix.m[3][2]
		- matrix.m[0][3] * matrix.m[1][2] * matrix.m[3][1] - matrix.m[0][2] * matrix.m[1][1] * matrix.m[3][3] - matrix.m[0][1] * matrix.m[1][3] * matrix.m[3][2];

	m.m[0][3] = -(matrix.m[0][1] * matrix.m[1][2] * matrix.m[2][3]) - (matrix.m[0][2] * matrix.m[1][3] * matrix.m[2][1]) - (matrix.m[0][3] * matrix.m[1][1] * matrix.m[2][2])
		+ matrix.m[0][3] * matrix.m[1][2] * matrix.m[2][1] + matrix.m[0][2] * matrix.m[1][1] * matrix.m[2][3] + matrix.m[0][1] * matrix.m[1][3] * matrix.m[2][2];


	m.m[1][0] = - (matrix.m[1][0] * matrix.m[2][2] * matrix.m[3][3]) - (matrix.m[1][2] * matrix.m[2][3] * matrix.m[3][0]) - (matrix.m[1][3] * matrix.m[2][0] * matrix.m[3][2])
		+ matrix.m[1][3] * matrix.m[2][2] * matrix.m[3][0] + matrix.m[1][2] * matrix.m[2][0] * matrix.m[3][3] + matrix.m[1][0] * matrix.m[2][3] * matrix.m[3][2];

	m.m[1][1] = matrix.m[0][0] * matrix.m[2][2] * matrix.m[3][3] + matrix.m[0][2] * matrix.m[2][3] * matrix.m[3][0] + matrix.m[0][3] * matrix.m[2][0] * matrix.m[3][2]
		- matrix.m[0][3] * matrix.m[2][2] * matrix.m[3][0] - matrix.m[0][2] * matrix.m[2][0] * matrix.m[3][3] - matrix.m[0][0] * matrix.m[2][3] * matrix.m[3][2];

	m.m[1][2] = -(matrix.m[0][0] * matrix.m[1][2] * matrix.m[3][3]) - (matrix.m[0][2] * matrix.m[1][3] * matrix.m[3][0]) - (matrix.m[0][3] * matrix.m[1][0] * matrix.m[3][2])
		+ matrix.m[0][3] * matrix.m[1][2] * matrix.m[3][0] + matrix.m[0][2] * matrix.m[1][0] * matrix.m[3][3] + matrix.m[0][0] * matrix.m[1][3] * matrix.m[3][2];

	m.m[1][3] = matrix.m[0][0] * matrix.m[1][2] * matrix.m[2][3] + matrix.m[0][2] * matrix.m[1][3] * matrix.m[2][0] + matrix.m[0][3] * matrix.m[1][0] * matrix.m[2][2]
		- matrix.m[0][3] * matrix.m[1][2] * matrix.m[2][0] - matrix.m[0][2] * matrix.m[1][0] * matrix.m[2][3] - matrix.m[0][0] * matrix.m[1][3] * matrix.m[2][2];


	m.m[2][0] = matrix.m[1][0] * matrix.m[2][1] * matrix.m[3][3] + matrix.m[1][1] * matrix.m[2][3] * matrix.m[3][0] + matrix.m[1][3] * matrix.m[2][0] * matrix.m[3][1]
		- matrix.m[1][3] * matrix.m[2][1] * matrix.m[3][0] - matrix.m[1][1] * matrix.m[2][0] * matrix.m[3][3] - matrix.m[1][0] * matrix.m[2][3] * matrix.m[3][1];

	m.m[2][1] = - (matrix.m[0][0] * matrix.m[2][1] * matrix.m[3][3]) - (matrix.m[0][1] * matrix.m[2][3] * matrix.m[3][0]) - (matrix.m[0][3] * matrix.m[2][0] * matrix.m[3][1])
		+ matrix.m[0][3] * matrix.m[2][1] * matrix.m[3][0] + matrix.m[0][1] * matrix.m[2][0] * matrix.m[3][3] + matrix.m[0][0] * matrix.m[2][3] * matrix.m[3][1];

	m.m[2][2] = matrix.m[0][0] * matrix.m[1][1] * matrix.m[3][3] + matrix.m[0][1] * matrix.m[1][3] * matrix.m[3][0] + matrix.m[0][3] * matrix.m[1][0] * matrix.m[3][1]
		- matrix.m[0][3] * matrix.m[1][1] * matrix.m[3][0] - matrix.m[0][1] * matrix.m[1][0] * matrix.m[3][3] - matrix.m[0][0] * matrix.m[1][3] * matrix.m[3][1];

	m.m[2][3] = - (matrix.m[0][0] * matrix.m[1][1] * matrix.m[2][3]) - (matrix.m[0][1] * matrix.m[1][3] * matrix.m[2][0]) - (matrix.m[0][3] * matrix.m[1][0] * matrix.m[2][1])
		+ matrix.m[0][3] * matrix.m[1][1] * matrix.m[2][0] + matrix.m[0][1] * matrix.m[1][0] * matrix.m[2][3] + matrix.m[0][0] * matrix.m[1][3] * matrix.m[2][1];


	m.m[3][0] = -(matrix.m[1][0] * matrix.m[2][1] * matrix.m[3][2]) - (matrix.m[1][1] * matrix.m[2][2] * matrix.m[3][0]) - (matrix.m[1][2] * matrix.m[2][0] * matrix.m[3][1])
		+ matrix.m[1][2] * matrix.m[2][1] * matrix.m[3][0] + matrix.m[1][1] * matrix.m[2][0] * matrix.m[3][2] + matrix.m[1][0] * matrix.m[2][2] * matrix.m[3][1];

	m.m[3][1] = matrix.m[0][0] * matrix.m[2][1] * matrix.m[3][2] + matrix.m[0][1] * matrix.m[2][2] * matrix.m[3][0] + matrix.m[0][2] * matrix.m[2][0] * matrix.m[3][1]
		- matrix.m[0][2] * matrix.m[2][1] * matrix.m[3][0] - matrix.m[0][1] * matrix.m[2][0] * matrix.m[3][2] - matrix.m[0][0] * matrix.m[2][2] * matrix.m[3][1];

	m.m[3][2] = -(matrix.m[0][0] * matrix.m[1][1] * matrix.m[3][2]) - (matrix.m[0][1] * matrix.m[1][2] * matrix.m[3][0]) - (matrix.m[0][2] * matrix.m[1][0] * matrix.m[3][1])
		+ matrix.m[0][2] * matrix.m[1][1] * matrix.m[3][0] + matrix.m[0][1] * matrix.m[1][0] * matrix.m[3][2] + matrix.m[0][0] * matrix.m[1][2] * matrix.m[3][1];

	m.m[3][3] = matrix.m[0][0] * matrix.m[1][1] * matrix.m[2][2] + matrix.m[0][1] * matrix.m[1][2] * matrix.m[2][0] + matrix.m[0][2] * matrix.m[1][0] * matrix.m[2][1]
		- matrix.m[0][2] * matrix.m[1][1] * matrix.m[2][0] - matrix.m[0][1] * matrix.m[1][0] * matrix.m[2][2] - matrix.m[0][0] * matrix.m[1][2] * matrix.m[2][1];


	float number;
	number = matrix.m[0][0] * matrix.m[1][1] * matrix.m[2][2] * matrix.m[3][3] + matrix.m[0][0] * matrix.m[1][2] * matrix.m[2][3] * matrix.m[3][1] + matrix.m[0][0] * matrix.m[1][3] * matrix.m[2][1] * matrix.m[3][2]
		- (matrix.m[0][0] * matrix.m[1][3] * matrix.m[2][2] * matrix.m[3][1]) - (matrix.m[0][0] * matrix.m[1][2] * matrix.m[2][1] * matrix.m[3][3]) - (matrix.m[0][0] * matrix.m[1][1] * matrix.m[2][3] * matrix.m[3][2])
		- (matrix.m[0][1] * matrix.m[1][0] * matrix.m[2][2] * matrix.m[3][3]) - (matrix.m[0][2] * matrix.m[1][0] * matrix.m[2][3] * matrix.m[3][1]) - (matrix.m[0][3] * matrix.m[1][0] * matrix.m[2][1] * matrix.m[3][2])
		+ matrix.m[0][3] * matrix.m[1][0] * matrix.m[2][2] * matrix.m[3][1] + matrix.m[0][2] * matrix.m[1][0] * matrix.m[2][1] * matrix.m[3][3] + matrix.m[0][1] * matrix.m[1][0] * matrix.m[2][3] * matrix.m[3][2]
		+ matrix.m[0][1] * matrix.m[1][2] * matrix.m[2][0] * matrix.m[3][3] + matrix.m[0][2] * matrix.m[1][3] * matrix.m[2][0] * matrix.m[3][1] + matrix.m[0][3] * matrix.m[1][1] * matrix.m[2][0] * matrix.m[3][2]
		- (matrix.m[0][3] * matrix.m[1][2] * matrix.m[2][0] * matrix.m[3][1]) - (matrix.m[0][2] * matrix.m[1][1] * matrix.m[2][0] * matrix.m[3][3]) - (matrix.m[0][1] * matrix.m[1][3] * matrix.m[2][0] * matrix.m[3][2])
		- (matrix.m[0][1] * matrix.m[1][2] * matrix.m[2][3] * matrix.m[3][0]) - (matrix.m[0][2] * matrix.m[1][3] * matrix.m[2][1] * matrix.m[3][0]) - (matrix.m[0][3] * matrix.m[1][1] * matrix.m[2][2] * matrix.m[3][0])
		+ matrix.m[0][3] * matrix.m[1][2] * matrix.m[2][1] * matrix.m[3][0] + matrix.m[0][2] * matrix.m[1][1] * matrix.m[2][3] * matrix.m[3][0] + matrix.m[0][1] * matrix.m[1][3] * matrix.m[2][2] * matrix.m[3][0];
	
	number = 1.0f / number;

	Matrix4x4 result{};

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			result.m[j][i] = number * m.m[j][i];
		}
	}

	return result;
}

Matrix4x4 MakeIdentity4x4()
{
	Matrix4x4 result{};

	for (int i = 0; i < 4; i++)
	{
		result.m[i][i] = 1;
	}

	return result;
}

Matrix4x4 MakeTranslateMatrix(const Vector3& translate)
{
	Matrix4x4 result=MakeIdentity4x4();

	result.m[3][0] = translate.x;
	result.m[3][1] = translate.y;
	result.m[3][2] = translate.z;

	return result;
}

Matrix4x4 MakeScaleMatrix(const Vector3& scale)
{
	Matrix4x4 result = MakeIdentity4x4();

	result.m[0][0] = scale.x;
	result.m[1][1] = scale.y;
	result.m[2][2] = scale.z;

	return result;
}

Matrix4x4 MakeRotateXMatrix(float rad)
{
	Matrix4x4 result = MakeIdentity4x4();

	result.m[1][1] = std::cosf(rad);
	result.m[1][2] = std::sinf(rad);
	result.m[2][1] = -std::sinf(rad);
	result.m[2][2] = std::cosf(rad);

	return result;
}

Matrix4x4 MakeRotateYMatrix(float rad)
{
	Matrix4x4 result = MakeIdentity4x4();

	result.m[0][0] = std::cosf(rad);
	result.m[0][2] = -std::sinf(rad);
	result.m[2][0] = std::sinf(rad);
	result.m[2][2] = std::cosf(rad);

	return result;
}

Matrix4x4 MakeRotateZMatrix(float rad)
{
	Matrix4x4 result = MakeIdentity4x4();

	result.m[0][0] = std::cosf(rad);
	result.m[0][1] = std::sinf(rad);
	result.m[1][0] = -std::sinf(rad);
	result.m[1][1] = std::cosf(rad);

	return result;
}

Matrix4x4 MakeRotateXYZMatrix(const Vector3& rota)
{

	Matrix4x4 result = MakeIdentity4x4();
	Matrix4x4 rotaxM = MakeRotateXMatrix(rota.x);
	Matrix4x4 rotayM = MakeRotateYMatrix(rota.y);
	Matrix4x4 rotazM = MakeRotateZMatrix(rota.z);
	result = Multiply(rotayM, rotazM);
	result = Multiply(rotaxM, result);

	return result;
}

Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate)
{
	Matrix4x4 result = MakeIdentity4x4();
	Matrix4x4 S = MakeScaleMatrix(scale);
	Matrix4x4 R = MakeRotateXYZMatrix(rotate);
	Matrix4x4 T = MakeTranslateMatrix(translate);

	for (int row = 0; row < 3; row++)
	{
		for (int col = 0; col < 3; col++)
		{
			result.m[row][col] = S.m[row][row] * R.m[row][col];
		}
	}
	result.m[3][0] = T.m[3][0];
	result.m[3][1] = T.m[3][1];
	result.m[3][2] = T.m[3][2];

	return result;
}

Matrix4x4 MakePerspectiveFovMatrix(float fovy, float aspectRation, float nearClip, float farClip)
{
	Matrix4x4 result{};

	result.m[0][0] = (1 / aspectRation) * (1 / std::tan(fovy / 2));
	result.m[1][1] = 1 / std::tan(fovy / 2);
	result.m[2][2] = farClip / (farClip - nearClip);
	result.m[2][3] = 1;
	result.m[3][2] = -(nearClip * farClip) / (farClip - nearClip);

	return result;
}

Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip)
{
	Matrix4x4 result = MakeIdentity4x4();

	result.m[0][0] = 2.0f / (right - left);
	result.m[1][1] = 2.0f / (top - bottom);
	result.m[2][2] = 1.0f / (farClip - nearClip);
	result.m[3][0] = (left + right) / (left - right);
	result.m[3][1] = (top + bottom) / (bottom - top);
	result.m[3][2] = nearClip / (nearClip - farClip);

	return result;
}

Matrix4x4 MakeViewportMat(float left, float top, float width, float height, float minDepth, float maxDepth)
{
	Matrix4x4 result = MakeIdentity4x4();

	result.m[0][0] = width / 2;
	result.m[1][1] = -(height / 2);
	result.m[2][2] = maxDepth - minDepth;
	result.m[3][0] = left + width / 2;
	result.m[3][1] = top + height / 2;
	result.m[3][2] = minDepth;

	return result;
}

Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m)
{
	Vector3 result{ v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0], v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1], v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] };

	return result;
}

Vector3 Lerp(const Vector3& v1, const Vector3& v2, float t) {
	Vector3 result;
	result = v1 + (v2 - v1) * t;
	return result;
}

Vector3 Cross(const Vector3& a, const Vector3& b) {
	return Vector3(
		a.y * b.z - a.z * b.y,  // X成分
		a.z * b.x - a.x * b.z,  // Y成分
		a.x * b.y - a.y * b.x   // Z成分
	);
}

float Clamp(float x, float min, float max) {
	return x < min ? min : (x > max ? max : x);
}

Vector3 CatmullRomPoint(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t) {

	const float s = 0.5f;

	float t2 = t * t;
	float t3 = t2 * t;

	Vector3 e3 = (p0 * (-1)) + (p1 * 3) - (p2 * 3) + p3;
	Vector3 e2 = p0 * 2 - (p1 * 5) + p2 * 4 + (p3 * (-1));
	Vector3 e1 = (p0 * (-1)) + p2;
	Vector3 e0 = p1 * 2;

	return (e3 * t3 + e2 * t2 + e1 * t + e0) * s;
}

Vector3 CatmullRom(const std::vector<Vector3>& control, float t) {
	assert(control.size() >= 4 && "制御点が4以下");

	size_t division = control.size() - 1;
	float areaWidth = 1.0f / division;

	float t_2 = std::fmod(t, areaWidth) * division;
	t_2 = Clamp(t_2, 0.0f, 1.0f);

	size_t index = static_cast<size_t>(t / areaWidth);
	index = std::min(index, control.size() - 2);

	size_t index0 = index - 1;
	size_t index1 = index;
	size_t index2 = index + 1;
	size_t index3 = index + 2;

	if (index == 0) {
		index0 = index1;
	}
	if (index3 >= control.size()) {
		index3 = index2;
	}
	const Vector3& p0 = control[index0];
	const Vector3& p1 = control[index1];
	const Vector3& p2 = control[index2];
	const Vector3& p3 = control[index3];
	return CatmullRomPoint(p0, p1, p2, p3, t_2);
}
