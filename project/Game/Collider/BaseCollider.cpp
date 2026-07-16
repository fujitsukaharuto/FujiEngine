#include "BaseCollider.h"
#include "Game/OriginObject/OriginGameObject.h"

using namespace Math;


BaseCollider::BaseCollider() {
}

void BaseCollider::InfoUpdate() {
	Matrix4x4 worldMatrix = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, info.pos + offset_);

	if (parent_) {
		worldMatrix = Multiply(worldMatrix, Math::RemoveScale(parent_->GetWorldMat()));
	}

	Vector3 position = { worldMatrix.m[3][0],worldMatrix.m[3][1],worldMatrix.m[3][2] };
	info.worldPos = position;
}

Vector3 BaseCollider::GetPos() {
	Matrix4x4 worldMatrix = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, info.pos + offset_);

	if (parent_) {
		worldMatrix = Multiply(worldMatrix, Math::RemoveScale(parent_->GetWorldMat()));
	}

	Vector3 position = { worldMatrix.m[3][0],worldMatrix.m[3][1],worldMatrix.m[3][2] };
	info.worldPos = position;
	return position;
}

Vector3 BaseCollider::GetWorldPos() {
	return GetPos();
}

OriginGameObject* BaseCollider::GetOwner() {
	return info.owner;
}