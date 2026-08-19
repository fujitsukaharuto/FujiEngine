#pragma once
#include <string>
#include <functional>
#include <vector>
#include "Engine/Math/Vector/Vector3.h"
#include "Engine/Graphics/Object/Object3d.h"

namespace GameObject { class GameObject; }

namespace Collision {

	/// <summary>
	/// コライダーの情報
	/// </summary>
	struct ColliderInfo {
		std::string tag;
		Math::Vector3 pos;
		Math::Vector3 worldPos;
		GameObject::GameObject* owner = nullptr; //コライダーを持つオブジェクトのポインタ
	};

	/// <summary>
	/// 衝突状態
	/// </summary>
	enum class CollisionState {
		CollisionEnter,
		CollisionStay,
		CollisionExit,
		None,
	};

	/// <summary>
	/// あたり判定用基底クラス
	/// </summary>
	class BaseCollider {
	public:
		BaseCollider();
		virtual ~BaseCollider() = default;

		virtual void SaveCollider(const std::string& filePath) = 0;

		/// <summary>
		/// 情報の更新
		/// </summary>
		void InfoUpdate();

		/// <summary>コライダーのワールド行列</summary>
		/// <remarks>
		/// 判定・デバッグ描画・ワールド座標は全てこの1本から導く。
		/// 経路を分けると描画される枠と実際の判定がズレて、当たり判定の調整ができなくなる
		/// </remarks>
		Math::Matrix4x4 GetWorldMatrix() const;

		//========================================================================*/
		//* 衝突履歴 (CollisionManager が使う)
		/// <summary>今この相手と衝突中か</summary>
		bool IsHitting(const BaseCollider* other) const;
		/// <summary>衝突開始として記録する</summary>
		/// <remarks>相手が破棄されても Exit を通知できるよう、開始時の情報を控えておく</remarks>
		void AddHit(const BaseCollider* other, const ColliderInfo& otherInfo);
		/// <summary>衝突終了として記録から外す</summary>
		void RemoveHit(const BaseCollider* other);
		/// <summary>生存していない相手を記録から落とし、その相手の情報を返す</summary>
		/// <remarks>返った分だけ Exit を通知する必要がある。相手は既に破棄されている可能性があるので控えた情報を使う</remarks>
		std::vector<ColliderInfo> RemoveHitsIfNot(const std::function<bool(const BaseCollider*)>& isAlive);

		//========================================================================*/
		//* Setter
		void SetParent(Math::Trans* parent) { parent_ = parent; }
		void SetTag(const std::string& tag) { info.tag = tag; }
		void SetOffset(const Math::Vector3& pos) { offset_ = pos; }
		void SetPos(const Math::Vector3& pos) { info.pos = pos; }
		void SetOwner(GameObject::GameObject* owner) { info.owner = owner; }
		void SetIsCollisonCheck(bool is) { isCollisionCheck_ = is; }

		//========================================================================*/
		//* Getter
		const std::string& GetTag()const { return info.tag; }
		Math::Vector3 GetPos();
		Math::Vector3 GetWorldPos();
		GameObject::GameObject* GetOwner();
		bool GetIsCollisonCheck() { return isCollisionCheck_; }

	protected:
		ColliderInfo info;
		Math::Trans* parent_ = nullptr;
		bool isCollisionCheck_ = true;
		Math::Vector3 offset_ = { 0.0f,0.0f,0.0f };

		const std::string kDirectoryPath_ = "resource/Json/Collider/";

	private:

		/// <summary>衝突中の相手1件</summary>
		struct HitRecord {
			const BaseCollider* other = nullptr;
			ColliderInfo info;	// 衝突開始時のスナップショット
		};

		std::vector<HitRecord> hitList_;
	};

}
