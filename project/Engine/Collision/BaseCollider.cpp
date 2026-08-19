#include "BaseCollider.h"
#include <algorithm>
#include "Engine/GameObject/GameObject.h"

using namespace Math;
using namespace Collision;


BaseCollider::BaseCollider() {
}

Matrix4x4 BaseCollider::GetWorldMatrix() const {
	Matrix4x4 worldMatrix = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, info.pos + offset_);

	if (parent_) {
		worldMatrix = Multiply(worldMatrix, Math::RemoveScale(parent_->GetWorldMat()));
	}
	return worldMatrix;
}

void BaseCollider::InfoUpdate() {
	const Matrix4x4 worldMatrix = GetWorldMatrix();
	info.worldPos = { worldMatrix.m[3][0],worldMatrix.m[3][1],worldMatrix.m[3][2] };
}

bool BaseCollider::IsHitting(const BaseCollider* other) const {
	return std::any_of(hitList_.begin(), hitList_.end(),
		[other](const HitRecord& record) { return record.other == other; });
}

void BaseCollider::AddHit(const BaseCollider* other, const ColliderInfo& otherInfo) {
	hitList_.push_back({ other, otherInfo });
}

void BaseCollider::RemoveHit(const BaseCollider* other) {
	std::erase_if(hitList_, [other](const HitRecord& record) { return record.other == other; });
}

std::vector<ColliderInfo> BaseCollider::RemoveHitsIfNot(const std::function<bool(const BaseCollider*)>& isAlive) {
	std::vector<ColliderInfo> lost;

	for (auto itr = hitList_.begin(); itr != hitList_.end();) {
		if (isAlive(itr->other)) {
			++itr;
		} else {
			lost.push_back(itr->info);
			itr = hitList_.erase(itr);
		}
	}
	return lost;
}

Vector3 BaseCollider::GetPos() {
	InfoUpdate();
	return info.worldPos;
}

Vector3 BaseCollider::GetWorldPos() {
	return GetPos();
}

GameObject::GameObject* BaseCollider::GetOwner() {
	return info.owner;
}
