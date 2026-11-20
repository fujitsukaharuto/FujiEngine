#include "CollisionManager.h"
#include <array>
#include <format>
#include "Engine/GameObject/GameObject.h"

using namespace Math;
using namespace Collision;

namespace {

	/// <summary>判定に加える生存中の GameObject のコライダーを順に渡す</summary>
	template <class Function>
	void ForEachLiveCollider(Function function) {
		for (GameObject::GameObject* object : GameObject::GameObject::GetAll()) {
			if (!object->IsActive() || object->IsDestroyed() || !object->IsCollisionActive()) {
				continue;
			}
			for (const auto& collider : object->GetColliders()) {
				function(collider.get());
			}
		}
	}

	std::array<std::string, kLayerCount>& LayerNames() {
		static std::array<std::string, kLayerCount> names = [] {
			std::array<std::string, kLayerCount> defaults;
			defaults[0] = "Default";
			for (uint32_t i = 1; i < kLayerCount; i++) {
				defaults[i] = std::format("Layer {}", i);
			}
			return defaults;
		}();
		return names;
	}
}


CollisionManager::CollisionManager() {
}

CollisionManager::~CollisionManager() {
}

void CollisionManager::AddGameObjectColliders() {
	ForEachLiveCollider([this](BaseCollider* collider) { colliders_.push_back(collider); });
}

bool CollisionManager::Intersects(const WorldShape& a, const WorldShape& b) {
	if (!Geometry::Intersects(a.bounds, b.bounds)) {
		return false;
	}
	switch (a.shape) {
	case ColliderShape::Box:
		switch (b.shape) {
		case ColliderShape::Box: return Geometry::Intersects(a.box, b.box);
		case ColliderShape::Sphere: return Geometry::Intersects(b.sphere, a.box);
		case ColliderShape::Capsule: return Geometry::Intersects(b.capsule, a.box);
		}
		break;
	case ColliderShape::Sphere:
		switch (b.shape) {
		case ColliderShape::Box: return Geometry::Intersects(a.sphere, b.box);
		case ColliderShape::Sphere: return Geometry::Intersects(a.sphere, b.sphere);
		case ColliderShape::Capsule: return Geometry::Intersects(b.capsule, a.sphere);
		}
		break;
	case ColliderShape::Capsule:
		switch (b.shape) {
		case ColliderShape::Box: return Geometry::Intersects(a.capsule, b.box);
		case ColliderShape::Sphere: return Geometry::Intersects(a.capsule, b.sphere);
		case ColliderShape::Capsule: return Geometry::Intersects(a.capsule, b.capsule);
		}
		break;
	}
	return false;
}

void CollisionManager::CheckCollisionPair(BaseCollider* A, BaseCollider* B) {
	if (!A || !B) {
		return;
	}
	const bool isColliding = A->GetIsCollisonCheck() && B->GetIsCollisonCheck() && BaseCollider::CanCollide(*A, *B) &&
		Intersects(A->GetWorldShape(), B->GetWorldShape());
	NotifyCollision(A, B, isColliding);
	NotifyCollision(B, A, isColliding);
}

void CollisionManager::NotifyCollision(BaseCollider* self, BaseCollider* other, bool isColliding) {

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

	for (BaseCollider* collider : colliders_) {
		// 相手は既に破棄されている可能性があるので、衝突開始時に控えた情報で通知する
		for (const ColliderInfo& lost : collider->RemoveHitsIfNot(
			[this](const BaseCollider* other) { return aliveColliders_.contains(other); })) {

			collider->SetState(CollisionState::CollisionExit);
			collider->OnCollision(lost);
		}
	}
}

void CollisionManager::CheckAllCollision() {

	if (colliders_.empty()) {
		return; // colliders_が空なら処理を終了
	}

	NotifyLostColliders();

	// ワールドの形はコライダーごとに1回だけ作る
	worldShapes_.clear();
	for (BaseCollider* collider : colliders_) {
		worldShapes_.push_back(collider->GetWorldShape());
	}

	for (size_t i = 0; i < colliders_.size(); ++i) {
		for (size_t j = i + 1; j < colliders_.size(); ++j) {
			BaseCollider* a = colliders_[i];
			BaseCollider* b = colliders_[j];
			// 判定が無効なペアは、重なっていても「離れている」として扱う(既に衝突中ならExitになる)
			const bool isColliding = a->GetIsCollisonCheck() && b->GetIsCollisonCheck() &&
				BaseCollider::CanCollide(*a, *b) && Intersects(worldShapes_[i], worldShapes_[j]);
			NotifyCollision(a, b, isColliding);
			NotifyCollision(b, a, isColliding);
		}
	}
}

bool CollisionManager::Raycast(const Vector3& origin, const Vector3& direction, float maxDistance,
	RaycastResult& result, uint32_t layerMask) {
	const float length = direction.Length();
	if (length <= 0.0f || maxDistance <= 0.0f) {
		return false;
	}
	const Ray ray{ origin, direction * (1.0f / length) };
	bool isHit = false;
	float nearest = maxDistance;
	ForEachLiveCollider([&](BaseCollider* collider) {
		if (!collider->GetIsCollisonCheck() || (collider->GetCollisionLayer() & layerMask) == 0) {
			return;
		}
		const WorldShape shape = collider->GetWorldShape();
		RayHit hit;
		bool isShapeHit = false;
		switch (shape.shape) {
		case ColliderShape::Box: isShapeHit = Geometry::Raycast(ray, shape.box, nearest, hit); break;
		case ColliderShape::Sphere: isShapeHit = Geometry::Raycast(ray, shape.sphere, nearest, hit); break;
		case ColliderShape::Capsule: isShapeHit = Geometry::Raycast(ray, shape.capsule, nearest, hit); break;
		}
		if (isShapeHit && hit.distance <= nearest) {
			nearest = hit.distance;
			result = { collider, collider->GetOwner(), hit.distance, hit.point, hit.normal };
			isHit = true;
		}
	});
	return isHit;
}

std::vector<BaseCollider*> CollisionManager::OverlapSphere(const Vector3& center, float radius, uint32_t layerMask) {
	std::vector<BaseCollider*> overlaps;
	WorldShape probe;
	probe.shape = ColliderShape::Sphere;
	probe.sphere = { center, radius };
	probe.bounds = probe.sphere;
	ForEachLiveCollider([&](BaseCollider* collider) {
		if (collider->GetIsCollisonCheck() && (collider->GetCollisionLayer() & layerMask) != 0 &&
			Intersects(probe, collider->GetWorldShape())) {
			overlaps.push_back(collider);
		}
	});
	return overlaps;
}

#ifdef _DEBUGMODE
void CollisionManager::DrawLiveColliders() {
	ForEachLiveCollider([](BaseCollider* collider) { collider->DrawCollider(); });
}
#endif // _DEBUGMODE

void CollisionManager::SetLayerName(uint32_t layer, const std::string& name) {
	if (layer < kLayerCount) {
		LayerNames()[layer] = name;
	}
}

const std::string& CollisionManager::GetLayerName(uint32_t layer) {
	static const std::string kOutOfRange = "?";
	return layer < kLayerCount ? LayerNames()[layer] : kOutOfRange;
}
