#pragma once
#include <cmath>
#include "Vector3.h"

namespace Math {
	/// <summary>
	/// Vector4クラス
	/// </summary>
	class Vector4 {
	public:

		float x;
		float y;
		float z;
		float w;

		/*----- コンストラクタ ----------------------------------------------------------*/
		constexpr Vector4() : x(0), y(0), z(0), w(0) {}
		constexpr Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
		constexpr Vector4(const Vector4& v) : x(v.x), y(v.y), z(v.z), w(v.w) {}


		Vector4 operator+(const Vector4& v) const { return Vector4(x + v.x, y + v.y, z + v.z, w + v.w); }
		Vector4 operator-(const Vector4& v) const { return Vector4(x - v.x, y - v.y, z - v.z, w - v.w); }

		Vector4& operator = (const Vector4& v) {
			x = v.x;
			y = v.y;
			z = v.z;
			w = v.w;
			return *this;
		}

		Vector3 xyz() const {
			return Vector3(x, y, z);
		}
	};
}