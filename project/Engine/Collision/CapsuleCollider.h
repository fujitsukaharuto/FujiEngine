#pragma once
#include "Engine/Collision/BaseCollider.h"

namespace Collision {

	/// <summary>
	/// カプセルのコライダー。高さは両端の半球を含む (Unity の CapsuleCollider と同じ)
	/// </summary>
	class CapsuleCollider : public BaseCollider {
	public:
		/// <summary>
		/// カプセルを伸ばす向き。親の軸
		/// </summary>
		enum class Axis {
			X,
			Y,
			Z,
		};

		CapsuleCollider() = default;
		~CapsuleCollider() override = default;

		ColliderShape GetShape() const override { return ColliderShape::Capsule; }
		WorldShape GetWorldShape() const override;
		void SaveCollider(const std::string& filePath) override;

	#ifdef _DEBUGMODE
		void DrawCollider() override;
	#endif // _DEBUGMODE

		void SetRadius(float radius) { radius_ = radius; }
		/// <param name="height">端から端まで。直径より短ければ球になる</param>
		void SetHeight(float height) { height_ = height; }
		void SetAxis(Axis axis) { axis_ = axis; }
		float GetRadius() const { return radius_; }
		float GetHeight() const { return height_; }
		Axis GetAxis() const { return axis_; }

	private:

		void ShapeGUI() override;

	private:

		float radius_ = 0.5f;
		float height_ = 2.0f;
		Axis axis_ = Axis::Y;
	};

}
