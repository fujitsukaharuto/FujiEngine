#pragma once
#include <array>
#include "Engine/Collision/BaseCollider.h"

namespace Collision {

	/// <summary>
	/// 箱のコライダー。名前は AABB だが親の回転に従う (判定は OBB)
	/// </summary>
	class AABBCollider : public BaseCollider {
	public:
		AABBCollider();
		~AABBCollider()override = default;

	public:

		ColliderShape GetShape() const override { return ColliderShape::Box; }
		WorldShape GetWorldShape() const override;

		/// <summary>
		///	衝突判定のデータ出力
		/// </summary>
		void SaveCollider(const std::string& filePath)override;

		//========================================================================*/
		//* Setter
		void SetWidth(float w) { width = w; }
		void SetHeight(float h) { height = h; }
		void SetDepth(float d) { depth = d; }

		//========================================================================*/
		//* Getter
		/// <summary>判定に使うワールド空間の8頂点</summary>
		std::array<Math::Vector3, 8> GetWorldVertices() const;

		float GetWidth() const { return width; }
		float GetHeight() const { return height; }
		float GetDepth() const { return depth; }


	#ifdef _DEBUGMODE
		/// <summary>判定ボリュームの枠を描く</summary>
		/// <remarks>GetWorldVertices 経由なので、描かれる枠と実際の判定は必ず一致する</remarks>
		void DrawCollider() override;
	#endif // _DEBUG

	private:

		void ShapeGUI() override;

	private:

		float width = 1.0f;
		float height = 1.0f;
		float depth = 1.0f;

	};

	/// <summary>箱のコライダー</summary>
	using BoxCollider = AABBCollider;

}
