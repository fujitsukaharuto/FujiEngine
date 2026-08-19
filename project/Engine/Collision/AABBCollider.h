#pragma once
#include <array>
#include "Engine/Collision/BaseCollider.h"

namespace Collision {

	/// <summary>
	/// あたり判定用クラス(AABB型)
	/// </summary>
	class AABBCollider : public BaseCollider {
	public:
		AABBCollider();
		~AABBCollider()override = default;

	public:

		void DebugGUI();

		/// <summary>
		///	衝突判定時に呼び出される関数
		/// </summary>
		/// <remarks>直前に SetState された状態に対応するコールバックだけを呼ぶ</remarks>
		void OnCollision(const ColliderInfo& other);

		/// <summary>
		///	衝突判定のデータ出力
		/// </summary>
		void SaveCollider(const std::string& filePath)override;

		//========================================================================*/
		//* イベントハンドラの設定
		void SetCollisionEnterCallback(std::function<void(const ColliderInfo&)> callback) {
			onCollisionEvents_[static_cast<int>(CollisionState::CollisionEnter)] = std::move(callback);
		}
		void SetCollisionStayCallback(std::function<void(const ColliderInfo&)> callback) {
			onCollisionEvents_[static_cast<int>(CollisionState::CollisionStay)] = std::move(callback);
		}
		void SetCollisionExitCallback(std::function<void(const ColliderInfo&)> callback) {
			onCollisionEvents_[static_cast<int>(CollisionState::CollisionExit)] = std::move(callback);
		}

		/// <summary>
		///	衝突状態の変更
		/// </summary>
		void SetState(CollisionState change) { state = change; }

		//========================================================================*/
		//* Setter
		void SetWidth(float w) { width = w; }
		void SetHeight(float h) { height = h; }
		void SetDepth(float d) { depth = d; }

		//========================================================================*/
		//* Getter
		/// <summary>衝突判定のデータ</summary>
		const ColliderInfo& GetInfo() const { return info; }
		const CollisionState& GetState()const { return state; }
		/// <summary>判定に使うワールド空間の8頂点</summary>
		std::array<Math::Vector3, 8> GetWorldVertices() const;

		float GetWidth() const { return width; }
		float GetHeight() const { return height; }
		float GetDepth() const { return depth; }


	#ifdef _DEBUGMODE
		/// <summary>判定ボリュームの枠を描く</summary>
		/// <remarks>GetWorldVertices 経由なので、描かれる枠と実際の判定は必ず一致する</remarks>
		void DrawCollider();
	#endif // _DEBUG


	private:

		CollisionState state = CollisionState::None;

		std::array<std::function<void(const ColliderInfo&)>, 3> onCollisionEvents_;

		float width = 1.0f;
		float height = 1.0f;
		float depth = 1.0f;

	};

}
