#include "BaseCollider.h"
#include "Game/OriginGameObject.h"

BaseCollider::BaseCollider() {
}

void BaseCollider::InfoUpdate() {
	Matrix4x4 worldMatrix = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, info.pos);

	if (parent_) {
		const Matrix4x4& parentWorldMatrix = parent_->GetWorldMat();
		worldMatrix = Multiply(worldMatrix, parentWorldMatrix);
	}

	Vector3 position = { worldMatrix.m[3][0],worldMatrix.m[3][1],worldMatrix.m[3][2] };
	info.worldPos = position;
}

Vector3 BaseCollider::GetPos() {
	Matrix4x4 worldMatrix = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, info.pos);

	if (parent_) {
		const Matrix4x4& parentWorldMatrix = parent_->GetWorldMat();
		worldMatrix = Multiply(worldMatrix, parentWorldMatrix);
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