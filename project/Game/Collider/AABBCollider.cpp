#include "AABBCollider.h"
#include <json.hpp>
#include "Engine/Model/Line/Line3dDrawer.h"
#include "Engine/ImGuiManager/ImGuiManager.h"
#include "Engine/Serialize/JsonSerializer.h"

using namespace Core;
using namespace Graphics;
using namespace Math;


AABBCollider::AABBCollider() {
}

void AABBCollider::DebugGUI() {
#ifdef _DEBUGMODE
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
	// onCollisionEvents_ は Enter/Stay/Exit の3つ分しかないので None は必ず弾く
	if (state == CollisionState::None) {
		return;
	}

	if (onCollisionEvents_[static_cast<int>(state)]) {
		onCollisionEvents_[static_cast<int>(state)](other);
	}
	if (state == CollisionState::CollisionExit) {
		state = CollisionState::None;
	}
}

void AABBCollider::SaveCollider(const std::string& filePath) {
#ifdef _DEBUGMODE
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

	// 判定と同じワールド行列を使う(別経路で組み立てると枠と判定がズレる)
	const Matrix4x4 worldMatrix = GetWorldMatrix();

	// 各頂点をワールド空間に変換
	for (int i = 0; i < 8; ++i) {
		worldVertices[i] = Transform(localVertices[i], worldMatrix);
	}

	return worldVertices;
}

#ifdef _DEBUGMODE
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