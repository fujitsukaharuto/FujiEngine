#pragma once
#include "Game/Collider/BaseCollider.h"
#include <array>
#include <unordered_set>
#include <vector>


class AABBCollider;

/// <summary>
/// OBBのデータ
/// </summary>
struct OBB {

	Math::Vector3 size;		// ハーフサイズ
	Math::Vector3 center;
	/// <summary>正規直交基底。ワールド行列から直接取る(オイラー角を経由すると非可逆で判定がズレる)</summary>
	std::array<Math::Vector3, 3> axes;
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
	void CheckCollisionPair(BaseCollider* A, BaseCollider* B);
	/// <summary>
	///	全ての衝突判定チェック
	/// </summary>
	void CheckAllCollision();

	/// <summary>
	/// Listに追加
	/// </summary>
	void AddCollider(BaseCollider* collider) { colliders_.push_back(collider); }

	void Reset() { colliders_.clear(); }

private:

	/// <summary>
	/// AABBをOBBに変換
	/// </summary>
	OBB ConvertAABBToOBB(const AABBCollider* aabb);

	/// <summary>
	/// 衝突のチェック
	/// </summary>
	bool checkAABBCollision(AABBCollider* A, AABBCollider* B);// 一度OBBの当たり判定に変えている

	/// <summary>
	/// self から見た other との衝突状態を判定し、対応するコールバックを呼ぶ
	/// </summary>
	void NotifyCollision(AABBCollider* self, AABBCollider* other, bool isColliding);

	/// <summary>
	/// 登録が外れた相手との衝突を終了させる
	/// </summary>
	void NotifyLostColliders();

private:

	/// <remarks>毎フレーム Reset→AddCollider で作り直す。clear() は容量を保つので確保は初回だけで済む</remarks>
	std::vector<BaseCollider*> colliders_;

	// 以下は CheckAllCollision の作業用。メンバに持つのは毎フレームの再確保を避けるため
	std::vector<AABBCollider*> aabbColliders_;
	std::unordered_set<const BaseCollider*> aliveColliders_;

};
