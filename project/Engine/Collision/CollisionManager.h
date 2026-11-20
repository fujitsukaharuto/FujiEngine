#pragma once
#include "Engine/Collision/BaseCollider.h"
#include <string>
#include <unordered_set>
#include <vector>

namespace Collision {

	/// <summary>
	/// レイキャストで最初に当たったもの
	/// </summary>
	struct RaycastResult {
		BaseCollider* collider = nullptr;
		GameObject::GameObject* owner = nullptr;
		float distance = 0.0f;
		Math::Vector3 point;
		Math::Vector3 normal;	// 当たった面の外向き
	};

	/// <summary>
	/// あたり判定管理クラス
	/// </summary>
	class CollisionManager {
	public:
		CollisionManager();
		~CollisionManager();

	public:
		/// <summary>
		/// 衝突のチェック
		/// </summary>
		/// <remarks>登録とは関係なく2つだけを判定する入口</remarks>
		void CheckCollisionPair(BaseCollider* A, BaseCollider* B);
		/// <summary>
		///	全ての衝突判定チェック
		/// </summary>
		void CheckAllCollision();

		/// <summary>
		/// Listに追加
		/// </summary>
		void AddCollider(BaseCollider* collider) { colliders_.push_back(collider); }
		/// <summary>生存中の GameObject が持つコライダーを全て登録する</summary>
		/// <remarks>止めている・破棄予約済み・IsCollisionActive() が false のものは飛ばす。Reset はしない</remarks>
		void AddGameObjectColliders();

		void Reset() { colliders_.clear(); }

		//========================================================================*/
		//* 問い合わせ。いつ呼んでもよい (生存中の GameObject をその場で調べる)

		/// <summary>2つのワールドの形が重なっているか</summary>
		static bool Intersects(const WorldShape& a, const WorldShape& b);

		/// <summary>レイに最初に当たったコライダー</summary>
		/// <remarks>原点が中にあるコライダーには当たらない。判定を切っているコライダーは見ない</remarks>
		/// <param name="direction">向き。長さは問わない</param>
		/// <param name="layerMask">当てるレイヤー (ビットの組)</param>
		static bool Raycast(const Math::Vector3& origin, const Math::Vector3& direction, float maxDistance,
			RaycastResult& result, uint32_t layerMask = kAllLayers);

		/// <summary>球に重なっているコライダー</summary>
		static std::vector<BaseCollider*> OverlapSphere(const Math::Vector3& center, float radius, uint32_t layerMask = kAllLayers);

	#ifdef _DEBUGMODE
		/// <summary>生存中の GameObject のコライダーの線を全部描く</summary>
		static void DrawLiveColliders();
	#endif // _DEBUGMODE

		//========================================================================*/
		//* レイヤーの名前 (GUI に出すだけ。判定はビットで行う)
		static void SetLayerName(uint32_t layer, const std::string& name);
		static const std::string& GetLayerName(uint32_t layer);

	private:

		/// <summary>
		/// self から見た other との衝突状態を判定し、対応するコールバックを呼ぶ
		/// </summary>
		void NotifyCollision(BaseCollider* self, BaseCollider* other, bool isColliding);

		/// <summary>
		/// 登録が外れた相手との衝突を終了させる
		/// </summary>
		void NotifyLostColliders();

	private:

		/// <remarks>毎フレーム Reset→AddCollider で作り直す</remarks>
		std::vector<BaseCollider*> colliders_;

		// 以下は CheckAllCollision の作業用。メンバに持つのは毎フレームの再確保を避けるため
		std::vector<WorldShape> worldShapes_;
		std::unordered_set<const BaseCollider*> aliveColliders_;

	};

}
