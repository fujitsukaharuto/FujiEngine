#include "BaseCollider.h"
#include "Game/OriginGameObject.h"

BaseCollider::BaseCollider() {
}

void BaseCollider::InfoUpdate() {
	Matrix4x4 worldMatrix = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, info.pos + offset_);

	if (parent_) {
		const Matrix4x4& parentWorldMatrix = parent_->GetWorldMat();
		// スケール成分を除去した親ワールド行列を作成
		Matrix4x4 noScaleParentMatrix = parentWorldMatrix;

		// 各軸ベクトルの長さ（スケール）を計算
		Vector3 xAxis = { parentWorldMatrix.m[0][0], parentWorldMatrix.m[1][0], parentWorldMatrix.m[2][0] };
		Vector3 yAxis = { parentWorldMatrix.m[0][1], parentWorldMatrix.m[1][1], parentWorldMatrix.m[2][1] };
		Vector3 zAxis = { parentWorldMatrix.m[0][2], parentWorldMatrix.m[1][2], parentWorldMatrix.m[2][2] };

		float xLen = Vector3::Length(xAxis);
		float yLen = Vector3::Length(yAxis);
		float zLen = Vector3::Length(zAxis);

		// 正規化（スケールを除去）
		for (int i = 0; i < 3; ++i) {
			noScaleParentMatrix.m[i][0] /= xLen;
			noScaleParentMatrix.m[i][1] /= yLen;
			noScaleParentMatrix.m[i][2] /= zLen;
		}

		// 変換はそのまま（位置は影響受けてOKなら）
		worldMatrix = Multiply(worldMatrix, noScaleParentMatrix);
	}

	Vector3 position = { worldMatrix.m[3][0],worldMatrix.m[3][1],worldMatrix.m[3][2] };
	info.worldPos = position;
}

Vector3 BaseCollider::GetPos() {
	Matrix4x4 worldMatrix = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, info.pos + offset_);

	if (parent_) {
		const Matrix4x4& parentWorldMatrix = parent_->GetWorldMat();
		// スケール成分を除去した親ワールド行列を作成
		Matrix4x4 noScaleParentMatrix = parentWorldMatrix;

		// 各軸ベクトルの長さ（スケール）を計算
		Vector3 xAxis = { parentWorldMatrix.m[0][0], parentWorldMatrix.m[1][0], parentWorldMatrix.m[2][0] };
		Vector3 yAxis = { parentWorldMatrix.m[0][1], parentWorldMatrix.m[1][1], parentWorldMatrix.m[2][1] };
		Vector3 zAxis = { parentWorldMatrix.m[0][2], parentWorldMatrix.m[1][2], parentWorldMatrix.m[2][2] };

		float xLen = Vector3::Length(xAxis);
		float yLen = Vector3::Length(yAxis);
		float zLen = Vector3::Length(zAxis);

		// 正規化（スケールを除去）
		for (int i = 0; i < 3; ++i) {
			noScaleParentMatrix.m[i][0] /= xLen;
			noScaleParentMatrix.m[i][1] /= yLen;
			noScaleParentMatrix.m[i][2] /= zLen;
		}

		// 変換はそのまま（位置は影響受けてOKなら）
		worldMatrix = Multiply(worldMatrix, noScaleParentMatrix);
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