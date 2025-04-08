#pragma once
#include <cmath>
#include <cstring>

class Matrix3x3 final {
public:

	float m[3][3];

	/*----- コンストラクタ ----------------------------------------------------------*/
	Matrix3x3() {
		std::memset(m, 0, sizeof(m));
	}

	// 単位行列を生成
	static Matrix3x3 MakeIdentity3x3() {
		Matrix3x3 result;
		for (int i = 0; i < 3; ++i) {
			result.m[i][i] = 1.0f;
		}
		return result;
	}

};