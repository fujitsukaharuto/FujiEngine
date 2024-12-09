#pragma once
#include "Game/Collider/BaseCollider.h"
#include <list>


class AABBCollider;

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

	bool checkAABBCollision(AABBCollider* A, AABBCollider* B);

private:

	std::list<BaseCollider*> colliders_;

};
