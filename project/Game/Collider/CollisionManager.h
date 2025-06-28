#pragma once
#include "Game/Collider/BaseCollider.h"
#include <list>


class AABBCollider;

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

class CollisionManager {
public:
	CollisionManager();
	~CollisionManager();

public:

	void CheckCollisionPair(BaseCollider* A, BaseCollider* B);
	void CheckAllCollision();

	void AddCollider(BaseCollider* collider) { colliders_.push_back(collider); }

	void Reset() { colliders_.clear(); }

private:

	OBB ConvertAABBToOBB(const AABBCollider* aabb);

	bool checkAABBCollision(AABBCollider* A, AABBCollider* B);// 一度OBBの当たり判定に変えている

private:

	std::list<BaseCollider*> colliders_;

};
