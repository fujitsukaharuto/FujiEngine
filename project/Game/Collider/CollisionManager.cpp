#include "CollisionManager.h"
#include "AABBCollider.h"

CollisionManager::CollisionManager() {
}

CollisionManager::~CollisionManager() {
}

void CollisionManager::CheckCollisionPair(BaseCollider* A, BaseCollider* B) {

	AABBCollider* aabbA = dynamic_cast<AABBCollider*>(A);
	AABBCollider* aabbB = dynamic_cast<AABBCollider*>(B);

	if (aabbA && aabbB) {
		bool isColliding = checkAABBCollision(aabbA, aabbB);

		auto& hitListA = aabbA->hitList_;
		auto& hitListB = aabbB->hitList_;

		if (isColliding) {
			// AとBが初めて衝突した場合
			if (std::find(hitListA.begin(), hitListA.end(), B) == hitListA.end()) {
				hitListA.push_back(B);
				aabbA->SetState(CollisionState::collisionEnter);
			}
			else {
				aabbA->SetState(CollisionState::collisionStay);
			}

			if (std::find(hitListB.begin(), hitListB.end(), A) == hitListB.end()) {
				hitListB.push_back(A);
				aabbB->SetState(CollisionState::collisionEnter);
			}
			else {
				aabbB->SetState(CollisionState::collisionStay);
			}
		}
		else {
			// 衝突が終了した場合
			if (std::find(hitListA.begin(), hitListA.end(), B) != hitListA.end()) {
				hitListA.remove(B);
				aabbA->SetState(CollisionState::collisionExit);
			}
			if (std::find(hitListB.begin(), hitListB.end(), A) != hitListB.end()) {
				hitListB.remove(A);
				aabbB->SetState(CollisionState::collisionExit);
			}
		}

		// 衝突情報を通知
		aabbA->OnCollision({ B->GetTag(), B->GetPos() });
		aabbB->OnCollision({ A->GetTag(), A->GetPos() });
	}

}

void CollisionManager::CheckAllCollision() {

	if (colliders_.empty()) {
		return; // colliders_が空なら処理を終了
	}

	std::list<BaseCollider*>::iterator itrA = colliders_.begin();
	for (; itrA != colliders_.end(); ++itrA) {
		BaseCollider* colliderA = *itrA;

		std::list<BaseCollider*>::iterator itrB = itrA;
		itrB++;

		for (; itrB != colliders_.end(); ++itrB) {
			BaseCollider* colliderB = *itrB;

			CheckCollisionPair(colliderA, colliderB);
		}
	}
}

bool CollisionManager::checkAABBCollision(AABBCollider* A, AABBCollider* B) {

	// Aの情報を取得
	Vector3 posA = A->GetPos();
	float halfWidthA = A->GetWidth() / 2.0f;
	float halfHeightA = A->GetHeight() / 2.0f;
	float halfDepthA = A->GetDepth() / 2.0f;

	// Bの情報を取得
	Vector3 posB = B->GetPos();
	float halfWidthB = B->GetWidth() / 2.0f;
	float halfHeightB = B->GetHeight() / 2.0f;
	float halfDepthB = B->GetDepth() / 2.0f;

	// AABB衝突判定
	return (
		std::abs(posA.x - posB.x) <= (halfWidthA + halfWidthB) &&
		std::abs(posA.y - posB.y) <= (halfHeightA + halfHeightB) &&
		std::abs(posA.z - posB.z) <= (halfDepthA + halfDepthB)
		);
}
