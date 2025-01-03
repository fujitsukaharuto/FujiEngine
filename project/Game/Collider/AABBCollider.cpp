#include "AABBCollider.h"
#include "Model/Line3dDrawer.h"

AABBCollider::AABBCollider() {
}

void AABBCollider::OnCollision(const ColliderInfo& other) {
	switch (state) {
	case CollisionState::collisionEnter:
		if (onCollisionEnter) {
			OnCollisionEnter(other);
		}
		break;
	case CollisionState::collisionStay:
		if (onCollisionStay) {
			OnCollisionStay(other);
		}
		break;
	case CollisionState::collisionExit:
		if (onCollisionExit) {
			OnCollisionExit(other);
		}
		break;
	case CollisionState::None:

		break;
	default:
		break;
	}
}


void AABBCollider::OnCollisionEnter(const ColliderInfo& other) {
	if (onCollisionEnter) {
		onCollisionEnter(other);
	}
}

void AABBCollider::OnCollisionStay(const ColliderInfo& other) {
	if (onCollisionStay) {
		onCollisionStay(other);
	}
}

void AABBCollider::OnCollisionExit(const ColliderInfo& other) {
	if (onCollisionExit) {
		onCollisionExit(other);
	}
}

#ifdef _DEBUG
void AABBCollider::DrawCollider() {

	// 半サイズを計算
	float halfWidth = width / 2.0f;
	float halfHeight = height / 2.0f;
	float halfDepth = depth / 2.0f;

	// 8つの頂点を計算
	Vector3 v1 = { info.worldPos.x - halfWidth, info.worldPos.y - halfHeight, info.worldPos.z - halfDepth };
	Vector3 v2 = { info.worldPos.x + halfWidth, info.worldPos.y - halfHeight, info.worldPos.z - halfDepth };
	Vector3 v3 = { info.worldPos.x + halfWidth, info.worldPos.y + halfHeight, info.worldPos.z - halfDepth };
	Vector3 v4 = { info.worldPos.x - halfWidth, info.worldPos.y + halfHeight, info.worldPos.z - halfDepth };
	Vector3 v5 = { info.worldPos.x - halfWidth, info.worldPos.y - halfHeight, info.worldPos.z + halfDepth };
	Vector3 v6 = { info.worldPos.x + halfWidth, info.worldPos.y - halfHeight, info.worldPos.z + halfDepth };
	Vector3 v7 = { info.worldPos.x + halfWidth, info.worldPos.y + halfHeight, info.worldPos.z + halfDepth };
	Vector3 v8 = { info.worldPos.x - halfWidth, info.worldPos.y + halfHeight, info.worldPos.z + halfDepth };

	// 線を描画
	Line3dDrawer::GetInstance()->DrawLine3d(v1, v2, { 1.0f,1.0f,1.0f,1.0f }); // 底面の線
	Line3dDrawer::GetInstance()->DrawLine3d(v2, v3, { 1.0f,1.0f,1.0f,1.0f });
	Line3dDrawer::GetInstance()->DrawLine3d(v3, v4, { 1.0f,1.0f,1.0f,1.0f });
	Line3dDrawer::GetInstance()->DrawLine3d(v4, v1, { 1.0f,1.0f,1.0f,1.0f });

	Line3dDrawer::GetInstance()->DrawLine3d(v5, v6, { 1.0f,1.0f,1.0f,1.0f }); // 上面の線
	Line3dDrawer::GetInstance()->DrawLine3d(v6, v7, { 1.0f,1.0f,1.0f,1.0f });
	Line3dDrawer::GetInstance()->DrawLine3d(v7, v8, { 1.0f,1.0f,1.0f,1.0f });
	Line3dDrawer::GetInstance()->DrawLine3d(v8, v5, { 1.0f,1.0f,1.0f,1.0f });

	Line3dDrawer::GetInstance()->DrawLine3d(v1, v5, { 1.0f,1.0f,1.0f,1.0f }); // 側面の線
	Line3dDrawer::GetInstance()->DrawLine3d(v2, v6, { 1.0f,1.0f,1.0f,1.0f });
	Line3dDrawer::GetInstance()->DrawLine3d(v3, v7, { 1.0f,1.0f,1.0f,1.0f });
	Line3dDrawer::GetInstance()->DrawLine3d(v4, v8, { 1.0f,1.0f,1.0f,1.0f });

}
#endif // _DEBUG


