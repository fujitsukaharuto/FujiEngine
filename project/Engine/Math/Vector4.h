#pragma once
/// <summary>
/// 4次元ベクトル
/// </summary>
struct Vector4{
	float x;
	float y;
	float z;
	float w;

	// コンストラクタ
	Vector4() : x(0), y(0), z(0),w(0) {}
	Vector4(float numX, float numY, float numZ,float numW) : x(numX), y(numY), z(numZ) ,w(numW){}

	// ベクトル加算
	Vector4 operator+(const Vector4& other) const;
};

Vector4 Normalize(const Vector4& v);  //正規化

float Length(const Vector4& v);

