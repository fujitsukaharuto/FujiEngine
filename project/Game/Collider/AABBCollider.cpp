#include "AABBCollider.h"
#include "Model/Line3dDrawer.h"
#include "ImGuiManager/ImGuiManager.h"
#include "Engine/Editor/JsonSerializer.h"

AABBCollider::AABBCollider() {
}

void AABBCollider::DebugGUI() {
#ifdef _DEBUG
	ImGui::Indent();
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Selected;
	if (ImGui::TreeNodeEx("Collider", flags)) {
		ImGui::DragFloat3("position", &info.pos.x, 0.1f);
		ImGui::DragFloat3("offset", &offset_.x, 0.1f);
		ImGui::DragFloat("width", &width, 0.1f);
		ImGui::DragFloat("height", &height, 0.1f);
		ImGui::DragFloat("depth", &depth, 0.1f);
		ImGui::Checkbox("isCollision", &isCollisionCheck_);
		static std::string colliderFileName = "colliderData";
		char buffer[128];
		strncpy_s(buffer, sizeof(buffer), colliderFileName.c_str(), _TRUNCATE);
		// 編集
		if (ImGui::InputText("##filename", buffer, IM_ARRAYSIZE(buffer))) {
			colliderFileName = buffer; // 編集結果を std::string に戻す
		}
		ImGui::SameLine();
		if (ImGui::Button("saveCollider")) {
			SaveCollider(colliderFileName);
		}
		InfoUpdate();
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

void AABBCollider::SaveCollider(const std::string& filePath) {
#ifdef _DEBUG
	nlohmann::json json;

	json["position"] = {
		info.pos.x,
		info.pos.y,
		info.pos.z,
	};
	json["offset"] = {
		offset_.x,
		offset_.y,
		offset_.z,
	};

	json["size"]["width"] = width;
	json["size"]["height"] = height;
	json["size"]["depth"] = depth;
	

	nlohmann::json finalJson;
	finalJson["collider"] = json;

	std::filesystem::create_directories(kDirectoryPath_);
	std::filesystem::path fullPath = kDirectoryPath_ + filePath;

	JsonSerializer::SerializeJsonData(finalJson, fullPath.string());
#endif // _DEBUG
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

	Matrix4x4 worldMatrix = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, info.pos + offset_);
	if (parent_) {
		const Matrix4x4& parentWorldMatrix = parent_->GetNoneScaleWorldMat();
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

	// 各頂点をワールド空間に変換
	for (int i = 0; i < 8; ++i) {
		worldVertices[i] = Transform(localVertices[i], worldMatrix);
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

	/*// 半サイズを計算
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
	Line3dDrawer::GetInstance()->DrawLine3d(v4, v8, { 1.0f,1.0f,1.0f,1.0f });*/
}
#endif // _DEBUG