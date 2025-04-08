#pragma once

class Matrix2x2 final {
public:

	float m[2][2];

	// コンストラクタ
	Matrix2x2() {
		std::memset(m, 0, sizeof(m));
	}

	// 単位行列を生成
	static Matrix2x2 MakeIdentity2x2() {
		Matrix2x2 result;
		for (int i = 0; i < 2; ++i) {
			result.m[i][i] = 1.0f;
		}
		return result;
	}

};