#pragma once
#include <cmath>

/// <summary>
/// Vector3クラス
/// </summary>
class Vector3 {
public:

	float x;
	float y;
	float z;

	/*----- コンストラクタ ----------------------------------------------------------*/
	Vector3() : x(0), y(0), z(0) {};
	Vector3(float x, float y, float z) :x(x), y(y), z(z) {}
	Vector3(const Vector3& v) :x(v.x), y(v.y), z(v.z) {}

	/*----- オペレーター -----------------------------------------------------------*/
	Vector3 operator+()const { return *this; }
	Vector3 operator-()const { return Vector3(-x, -y, -z); }

	Vector3 operator*(float k) const { return Vector3(x * k, y * k, z * k); }
	Vector3 operator/(float k) const { return Vector3(x / k, y / k, z / k); }

	Vector3 operator+(const Vector3& v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
	Vector3 operator-(const Vector3& v)const { return Vector3(x - v.x, y - v.y, z - v.z); }

	Vector3& operator = (const Vector3& v);
	Vector3& operator += (const Vector3& v);
	Vector3& operator -= (const Vector3& v);

	bool operator==(const Vector3& other) const { return x == other.x && y == other.y && z == other.z; }
	bool operator!=(const Vector3& other) const { return !(*this == other); }

	float operator*(const Vector3& v) const { return (x * v.x) + (y * v.y) + (z * v.z); }

	/*----- 関数 ------------------------------------------------------------------*/
	/// <summary>ベクトルの長さ（大きさ）を求める</summary>
	float Length() const { return std::sqrtf((*this) * (*this)); }
	/// <summary>他のベクトルとの内積を求める</summary>
	float Dot(const Vector3& other)const { return *this * other; }
	/// <summary>このベクトルを正規化する</summary>
	Vector3 Normalize() const;
	/// <summary>他のベクトルとの外積を求める</summary>
	Vector3 Cross(const Vector3& other) const;

	/// <summary>全て０のVector3を取得</summary>
	static Vector3 GetZeroVec() { return { 0.0f,0.0f,0.0f }; }
	/// <summary>上方向を向くVector</summary>
	static Vector3 GetUpVec() { return { 0.0f,1.0f,0.0f }; }
	/// <summary>右方向を向くVector</summary>
	static Vector3 Right() { return { 1.0f,0.0f,0.0f }; }
	/// <summary>上方向を向くVector</summary>
	static Vector3 Up() { return { 0.0f,1.0f,0.0f }; }
	/// <summary>前方向を向くVector</summary>
	static Vector3 Forward() { return { 0.0f,0.0f,1.0f }; }
	/// <summary>kで全て埋めるVector</summary>
	static Vector3 FillVec(float k) { return { k,k,k }; }
	/// <summary>内積</summary>
	static float Dot(const Vector3& a, const Vector3& b) { return a * b; }
	/// <summary>Vectorの長さ</summary>
	static float Length(const Vector3& a) { return a.Length(); }
	/// <summary>AとBの距離</summary>
	static float Distance(const Vector3& a, const Vector3& b) { return (a - b).Length(); }
	/// <summary>Vectorの正規化</summary>
	static Vector3 Normalize(const Vector3& v) { return v.Normalize(); }
	/// <summary>外積</summary>
	static Vector3 Cross(const Vector3& v1, const Vector3& v2) { return v1.Cross(v2); }
};


inline Vector3 operator+(float k, const Vector3& v) {
	return Vector3(k + v.x, k + v.y, k + v.z);
}

inline Vector3 operator-(float k, const Vector3& v) {
	return Vector3(k - v.x, k - v.y, k - v.z);
}

inline Vector3 operator*(float k, const Vector3& v) {
	return Vector3(k * v.x, k * v.y, k * v.z);
}
