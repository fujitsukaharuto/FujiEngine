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

			if (!aabbA->GetIsCollisonCheck() || !aabbB->GetIsCollisonCheck()) {
				if (std::find(hitListA.begin(), hitListA.end(), B) == hitListA.end()) {
				} else {
					hitListA.remove(B);
					aabbA->SetState(CollisionState::CollisionExit);
					aabbA->OnCollision({ B->GetTag(), B->GetPos(),B->GetWorldPos(),B->GetOwner() });
				}

				if (std::find(hitListB.begin(), hitListB.end(), A) == hitListB.end()) {
				} else {
					hitListB.remove(A);
					aabbB->SetState(CollisionState::CollisionExit);
					aabbB->OnCollision({ A->GetTag(), A->GetPos(),A->GetWorldPos(),A->GetOwner() });
				}
				return;
			}

			// AとBが初めて衝突した場合
			if (std::find(hitListA.begin(), hitListA.end(), B) == hitListA.end()) {
				hitListA.push_back(B);
				aabbA->SetState(CollisionState::CollisionEnter);
				aabbA->OnCollision({ B->GetTag(), B->GetPos(),B->GetWorldPos(),B->GetOwner() });
			} else {
				aabbA->SetState(CollisionState::CollisionStay);
				aabbA->OnCollision({ B->GetTag(), B->GetPos(),B->GetWorldPos(),B->GetOwner() });
			}

			if (std::find(hitListB.begin(), hitListB.end(), A) == hitListB.end()) {
				hitListB.push_back(A);
				aabbB->SetState(CollisionState::CollisionEnter);
				aabbB->OnCollision({ A->GetTag(), A->GetPos(),A->GetWorldPos(),A->GetOwner() });
			} else {
				aabbB->SetState(CollisionState::CollisionStay);
				aabbB->OnCollision({ A->GetTag(), A->GetPos(),A->GetWorldPos(),A->GetOwner() });
			}
		} else {
			// 衝突が終了した場合
			if (std::find(hitListA.begin(), hitListA.end(), B) != hitListA.end()) {
				hitListA.remove(B);
				aabbA->SetState(CollisionState::CollisionExit);
				aabbA->OnCollision({ B->GetTag(), B->GetPos(),B->GetWorldPos(),B->GetOwner() });
			}
			if (std::find(hitListB.begin(), hitListB.end(), A) != hitListB.end()) {
				hitListB.remove(A);
				aabbB->SetState(CollisionState::CollisionExit);
				aabbB->OnCollision({ A->GetTag(), A->GetPos(),A->GetWorldPos(),A->GetOwner() });
			}
		}

	}

}

void CollisionManager::CheckAllCollision() {

	if (colliders_.empty()) {
		return; // colliders_が空なら処理を終了
	}

	for (auto& collider : colliders_) {
		AABBCollider* aabb = dynamic_cast<AABBCollider*>(collider);
		if (aabb) {
			auto& hitList = aabb->hitList_;
			hitList.remove_if([this, aabb](BaseCollider* other) {
				// コライダーリストに存在しない場合
				if (std::find(colliders_.begin(), colliders_.end(), other) == colliders_.end()) {
					// 衝突終了状態を設定
					aabb->SetState(CollisionState::CollisionExit);
					return true; // リストから削除
				}
				return false; // 残す
				});
		}
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

OBB CollisionManager::ConvertAABBToOBB(const AABBCollider* aabb) {
	OBB obb;

	// ワールド中心座標
	obb.center = aabb->GetInfo().worldPos;

	// ハーフサイズ（AABBのサイズをOBB構造に）
	obb.size = {
		aabb->GetWidth() / 2.0f,
		aabb->GetHeight() / 2.0f,
		aabb->GetDepth() / 2.0f
	};

	// 回転（親が持つ回転をそのまま渡す）
	obb.rotate = aabb->IsHaveParent() ? aabb->GetParentRotate() : Vector3(0.0f, 0.0f, 0.0f);

	return obb;
}

bool CollisionManager::checkAABBCollision(AABBCollider* A, AABBCollider* B) {

	/*// Aの情報を取得
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
		);*/

	OBB obbA = ConvertAABBToOBB(A);
	OBB obbB = ConvertAABBToOBB(B);

	auto CalculateProjection =
		[](const OBB& obb, const Vector3& axis, const Vector3* axes) -> float {
		return std::abs(obb.size.x * Vector3::Dot(axes[0], axis)) +
			std::abs(obb.size.y * Vector3::Dot(axes[1], axis)) +
			std::abs(obb.size.z * Vector3::Dot(axes[2], axis));
		};

	auto GetOBBAxes = [](const OBB& obb) -> std::array<Vector3, 3> {
		Matrix4x4 rotationMatrix = MakeRotateXYZMatrix(obb.rotate);
		return {
			Transform(Vector3(1.0f, 0.0f, 0.0f), rotationMatrix),
			Transform(Vector3(0.0f, 1.0f, 0.0f), rotationMatrix),
			Transform(Vector3(0.0f, 0.0f, 1.0f), rotationMatrix)
		};
		};

	auto obbAxesA = GetOBBAxes(obbA);
	auto obbAxesB = GetOBBAxes(obbB);

	std::vector<Vector3> axes;
	axes.insert(axes.end(), obbAxesA.begin(), obbAxesA.end());
	axes.insert(axes.end(), obbAxesB.begin(), obbAxesB.end());
	for (const auto& axisA : obbAxesA) {
		for (const auto& axisB : obbAxesB) {
			axes.push_back(Vector3::Cross(axisA, axisB));
		}
	}

	for (const auto& axis : axes) {
		if (axis.Length() < std::numeric_limits<float>::epsilon()) {
			continue;
		}

		Vector3 normalizedAxis = axis.Normalize();

		float obbAProjection = CalculateProjection(obbA, normalizedAxis, obbAxesA.data());
		float obbBProjection = CalculateProjection(obbB, normalizedAxis, obbAxesB.data());
		float distance = std::abs(Vector3::Dot(obbA.center - obbB.center, normalizedAxis));

		if (distance > obbAProjection + obbBProjection) {
			return false;
		}
	}

	return true;

}