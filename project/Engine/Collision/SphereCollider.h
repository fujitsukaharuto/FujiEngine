#pragma once
#include "Engine/Collision/BaseCollider.h"

namespace Collision {

	/// <summary>
	/// 球のコライダー
	/// </summary>
	class SphereCollider : public BaseCollider {
	public:
		SphereCollider() = default;
		~SphereCollider() override = default;

		ColliderShape GetShape() const override { return ColliderShape::Sphere; }
		WorldShape GetWorldShape() const override;
		void SaveCollider(const std::string& filePath) override;

	#ifdef _DEBUGMODE
		void DrawCollider() override;
	#endif // _DEBUGMODE

		void SetRadius(float radius) { radius_ = radius; }
		float GetRadius() const { return radius_; }

	private:

		void ShapeGUI() override;

	private:

		float radius_ = 0.5f;
	};

}
