#include "AABBCollider.h"
#include "Model/Line3dDrawer.h"
#include "ImGuiManager/ImGuiManager.h"

AABBCollider::AABBCollider() {
}

void AABBCollider::DebugGUI() {
#ifdef _DEBUG
	ImGui::Indent();
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Selected;
	if (ImGui::TreeNodeEx("Collider",flags)) {
	ImGui::DragFloat("width", &width, 0.1f);
	ImGui::DragFloat("height", &height, 0.1f);
	ImGui::DragFloat("depth", &depth, 0.1f);
	ImGui::Checkbox("isCollision", &isCollisionCheck_);
		ImGui::TreePop();
	}
	ImGui::Unindent();
#endif // _DEBUG
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

std::array<Vector3, 8> AABBCollider::GetWorldVertices() const {
	// 半サイズ
	float hw = width / 2.0f;
	float hh = height / 2.0f;
	float hd = depth / 2.0f;

	// ローカル空間の8頂点
	std::array<Vector3, 8> localVertices = {
		Vector3{-hw, -hh, -hd},
		Vector3{ hw, -hh, -hd},
		Vector3{ hw,  hh, -hd},
		Vector3{-hw,  hh, -hd},
		Vector3{-hw, -hh,  hd},
		Vector3{ hw, -hh,  hd},
		Vector3{ hw,  hh,  hd},
		Vector3{-hw,  hh,  hd},
	};

	std::array<Vector3, 8> worldVertices;

	Matrix4x4 worldMatrix;
	if (parent_) {
		worldMatrix = parent_->GetWorldMat(); // 回転含む
	} else {
		worldMatrix = MakeIdentity4x4(); // 回転なし
	}

	// 各頂点をワールド空間に変換
	for (int i = 0; i < 8; ++i) {
		worldVertices[i] = Transform(localVertices[i] + info.pos, worldMatrix);
	}

	return worldVertices;
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

	std::array<Vector3, 8> v = GetWorldVertices();

	// 線を描画
	Line3dDrawer::GetInstance()->DrawLine3d(v[0], v[1], { 1.0f,1.0f,1.0f,1.0f }); // 底面の線
	Line3dDrawer::GetInstance()->DrawLine3d(v[1], v[2], { 1.0f,1.0f,1.0f,1.0f });
	Line3dDrawer::GetInstance()->DrawLine3d(v[2], v[3], { 1.0f,1.0f,1.0f,1.0f });
	Line3dDrawer::GetInstance()->DrawLine3d(v[3], v[0], { 1.0f,1.0f,1.0f,1.0f });

	Line3dDrawer::GetInstance()->DrawLine3d(v[4], v[5], { 1.0f,1.0f,1.0f,1.0f }); // 上面の線
	Line3dDrawer::GetInstance()->DrawLine3d(v[5], v[6], { 1.0f,1.0f,1.0f,1.0f });
	Line3dDrawer::GetInstance()->DrawLine3d(v[6], v[7], { 1.0f,1.0f,1.0f,1.0f });
	Line3dDrawer::GetInstance()->DrawLine3d(v[7], v[4], { 1.0f,1.0f,1.0f,1.0f });

	Line3dDrawer::GetInstance()->DrawLine3d(v[0], v[4], { 1.0f,1.0f,1.0f,1.0f }); // 側面の線
	Line3dDrawer::GetInstance()->DrawLine3d(v[1], v[5], { 1.0f,1.0f,1.0f,1.0f });
	Line3dDrawer::GetInstance()->DrawLine3d(v[2], v[6], { 1.0f,1.0f,1.0f,1.0f });
	Line3dDrawer::GetInstance()->DrawLine3d(v[3], v[7], { 1.0f,1.0f,1.0f,1.0f });

}
#endif // _DEBUG