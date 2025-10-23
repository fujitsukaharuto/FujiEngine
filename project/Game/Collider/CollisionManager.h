#pragma once
#include "Game/Collider/BaseCollider.h"
#include <list>


class AABBCollider;

/// <summary>
/// OBBのデータ
/// </summary>
struct OBB {

	Vector3 size;
	Vector3 center;
	Vector3 rotate;

	static OBB Default() {
		OBB obb = {
			.size = {1.0f,1.0f,1.0f},
			.center = {0.0f,0.0f,0.0f},
			.rotate = {0.0f,0.0f,0.0f}
		};
		return obb;
	};
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

private:

	std::list<BaseCollider*> colliders_;

};
