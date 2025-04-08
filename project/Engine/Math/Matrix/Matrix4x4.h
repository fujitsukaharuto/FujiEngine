#pragma once
#include <cmath>
#include <cstring>

class Matrix4x4 final {
public:

	float m[4][4];

	/*----- コンストラクタ ----------------------------------------------------------*/
	Matrix4x4() {
		std::memset(m, 0, sizeof(m));
	}

	/*----- オペレーター -----------------------------------------------------------*/
	Matrix4x4 operator*(const Matrix4x4& other) const {
		Matrix4x4 result;
		for (int row = 0; row < 4; ++row) {
			for (int col = 0; col < 4; ++col) {
				result.m[row][col] = 0.0f;
				for (int k = 0; k < 4; ++k) {
					result.m[row][col] += m[row][k] * other.m[k][col];
				}
			}
		}
		return result;
	}

	/*----- 関数 ------------------------------------------------------------------*/
	static Matrix4x4 MakeIdentity4x4() {
		Matrix4x4 result;
		for (int i = 0; i < 4; ++i) {
			result.m[i][i] = 1.0f;
		}
		return result;
	}

	Matrix4x4 Transpose() const {
		Matrix4x4 result;
		for (int row = 0; row < 4; ++row) {
			for (int col = 0; col < 4; ++col) {
				result.m[row][col] = m[col][row];
			}
		}
		return result;
	}

};