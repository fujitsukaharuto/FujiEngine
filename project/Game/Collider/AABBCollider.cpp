#include "AABBCollider.h"
#include "Model/Line3dDrawer.h"
#include "ImGuiManager/ImGuiManager.h"

AABBCollider::AABBCollider() {
}

void AABBCollider::OnCollision(const ColliderInfo& other) {
	if (onCollisionEvents_[static_cast<int>(state)]) {
		onCollisionEvents_[static_cast<int>(state)](other);
		if (state == CollisionState::CollisionExit) {
			state = CollisionState::None;
		}
	}
	
	/*switch (state) {
	case CollisionState::CollisionEnter:
		if (onCollisionEvents_[static_cast<int>(CollisionState::CollisionEnter)]) {
			OnCollisionEnter(other);
		}
		break;
	case CollisionState::CollisionStay:
		if (onCollisionEvents_[static_cast<int>(CollisionState::CollisionStay)]) {
			OnCollisionStay(other);
		}
		break;
	case CollisionState::CollisionExit:
		if (onCollisionEvents_[static_cast<int>(CollisionState::CollisionExit)]) {
			OnCollisionExit(other);
		}
		break;
	case CollisionState::None:

		break;
	default:
		break;
	}*/
}


void AABBCollider::OnCollisionEnter(const ColliderInfo& other) {
	if (onCollisionEvents_[static_cast<int>(CollisionState::CollisionEnter)]) {
		onCollisionEvents_[static_cast<int>(CollisionState::CollisionEnter)](other);
	}
}

void AABBCollider::OnCollisionStay(const ColliderInfo& other) {
	if (onCollisionEvents_[static_cast<int>(CollisionState::CollisionStay)]) {
		onCollisionEvents_[static_cast<int>(CollisionState::CollisionStay)](other);
	}
}

void AABBCollider::OnCollisionExit(const ColliderInfo& other) {
	if (onCollisionEvents_[static_cast<int>(CollisionState::CollisionExit)]) {
		onCollisionEvents_[static_cast<int>(CollisionState::CollisionExit)](other);
	}
	state = CollisionState::None;
}

#ifdef _DEBUG
void AABBCollider::Debug() {
	std::string uniqueName = "AABBCollider##" + std::to_string(reinterpret_cast<uintptr_t>(this));
	ImGui::Begin(uniqueName.c_str());
	ImGui::DragFloat3("pos", &info.pos.x, 0.01f);
	ImGui::DragFloat("width", &width, 0.01f);
	ImGui::DragFloat("height", &height, 0.01f);
	ImGui::DragFloat("depth", &depth, 0.01f);
	ImGui::Checkbox("isCollision", &isCollisionCheck_);
	ImGui::End();
}

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