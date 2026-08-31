#include "CollisionManager.h"
#include "AABBCollider.h"

using namespace Math;
using namespace Collision;


CollisionManager::CollisionManager() {
}

CollisionManager::~CollisionManager() {
}

void CollisionManager::CheckCollisionPair(BaseCollider* A, BaseCollider* B) {

	AABBCollider* aabbA = dynamic_cast<AABBCollider*>(A);
	AABBCollider* aabbB = dynamic_cast<AABBCollider*>(B);

	if (!aabbA || !aabbB) {
		return;
	}

	CheckAABBPair(aabbA, aabbB);
}

void CollisionManager::CheckAABBPair(AABBCollider* A, AABBCollider* B) {

	// 判定が無効なペアは、重なっていても「離れている」として扱う(既に衝突中ならExitになる)
	const bool isColliding =
		A->GetIsCollisonCheck() &&
		B->GetIsCollisonCheck() &&
		checkAABBCollision(A, B);

	NotifyCollision(A, B, isColliding);
	NotifyCollision(B, A, isColliding);
}

void CollisionManager::NotifyCollision(AABBCollider* self, AABBCollider* other, bool isColliding) {

	const bool wasHitting = self->IsHitting(other);

	// 大半のペアは「触れていないし触れていなかった」なので、情報を作る前に抜ける
	if (!isColliding && !wasHitting) {
		return;
	}

	const Vector3 otherPos = other->GetPos();
	const ColliderInfo otherInfo{ other->GetTag(), otherPos, otherPos, other->GetOwner() };

	if (isColliding) {
		if (wasHitting) {
			self->SetState(CollisionState::CollisionStay);
		} else {
			self->AddHit(other, otherInfo);
			self->SetState(CollisionState::CollisionEnter);
		}
	} else {
		self->RemoveHit(other);
		self->SetState(CollisionState::CollisionExit);
	}

	self->OnCollision(otherInfo);
}

void CollisionManager::NotifyLostColliders() {

	aliveColliders_.clear();
	aliveColliders_.insert(colliders_.begin(), colliders_.end());

	for (AABBCollider* aabb : aabbColliders_) {
		// 相手は既に破棄されている可能性があるので、衝突開始時に控えた情報で通知する
		for (const ColliderInfo& lost : aabb->RemoveHitsIfNot(
			[this](const BaseCollider* other) { return aliveColliders_.contains(other); })) {

			aabb->SetState(CollisionState::CollisionExit);
			aabb->OnCollision(lost);
		}
	}
}

void CollisionManager::CheckAllCollision() {

	if (colliders_.empty()) {
		return; // colliders_が空なら処理を終了
	}

	// 型の絞り込みはペアループの外で1回だけ行う(内側でやると dynamic_cast が O(n^2) 回走る)
	aabbColliders_.clear();
	for (BaseCollider* collider : colliders_) {
		if (AABBCollider* aabb = dynamic_cast<AABBCollider*>(collider)) {
			aabbColliders_.push_back(aabb);
		}
	}

	NotifyLostColliders();

	for (size_t i = 0; i < aabbColliders_.size(); ++i) {
		for (size_t j = i + 1; j < aabbColliders_.size(); ++j) {
			CheckAABBPair(aabbColliders_[i], aabbColliders_[j]);
		}
	}
}

OBB CollisionManager::ConvertAABBToOBB(const AABBCollider* aabb) {
	OBB obb;

	// キャッシュ済みの worldPos は InfoUpdate() を呼び忘れると古いままなので、その場で作り直す
	const Matrix4x4 world = aabb->GetWorldMatrix();

	// ワールド中心座標
	obb.center = { world.m[3][0],world.m[3][1],world.m[3][2] };

	// ハーフサイズ（AABBのサイズをOBB構造に）
	obb.size = {
		aabb->GetWidth() / 2.0f,
		aabb->GetHeight() / 2.0f,
		aabb->GetDepth() / 2.0f
	};

	// 回転（ワールド行列の基底をそのまま軸にする）
	obb.axes = {
		Vector3(world.m[0][0],world.m[0][1],world.m[0][2]).Normalize(),
		Vector3(world.m[1][0],world.m[1][1],world.m[1][2]).Normalize(),
		Vector3(world.m[2][0],world.m[2][1],world.m[2][2]).Normalize(),
	};

	return obb;
}

bool CollisionManager::checkAABBCollision(AABBCollider* A, AABBCollider* B) {

	const OBB obbA = ConvertAABBToOBB(A);
	const OBB obbB = ConvertAABBToOBB(B);

	auto CalculateProjection =
		[](const OBB& obb, const Vector3& axis) -> float {
		return std::abs(obb.size.x * Vector3::Dot(obb.axes[0], axis)) +
			std::abs(obb.size.y * Vector3::Dot(obb.axes[1], axis)) +
			std::abs(obb.size.z * Vector3::Dot(obb.axes[2], axis));
		};

	// 分離軸は 3 + 3 + 3x3 の15本で固定なので、毎フレームのヒープ確保は不要
	std::array<Vector3, 15> axes;
	size_t axisCount = 0;
	for (const auto& axisA : obbA.axes) {
		axes[axisCount++] = axisA;
	}
	for (const auto& axisB : obbB.axes) {
		axes[axisCount++] = axisB;
	}
	for (const auto& axisA : obbA.axes) {
		for (const auto& axisB : obbB.axes) {
			axes[axisCount++] = Vector3::Cross(axisA, axisB);
		}
	}

	for (size_t i = 0; i < axisCount; ++i) {
		const Vector3& axis = axes[i];
		if (axis.Length() < std::numeric_limits<float>::epsilon()) {
			continue;
		}

		const Vector3 normalizedAxis = axis.Normalize();

		const float obbAProjection = CalculateProjection(obbA, normalizedAxis);
		const float obbBProjection = CalculateProjection(obbB, normalizedAxis);
		const float distance = std::abs(Vector3::Dot(obbA.center - obbB.center, normalizedAxis));

		if (distance > obbAProjection + obbBProjection) {
			return false;
		}
	}

	return true;

}
