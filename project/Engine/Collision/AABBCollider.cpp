#include "AABBCollider.h"
#include <json.hpp>
#include "Engine/Graphics/Line/Line3dDrawer.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/Core/Serialize/JsonSerializer.h"

using namespace Core;
using namespace Graphics;
using namespace Math;
using namespace Collision;


AABBCollider::AABBCollider() {
}

void AABBCollider::ShapeGUI() {
#ifdef _DEBUGMODE
	ImGui::DragFloat("width", &width, 0.1f);
	ImGui::DragFloat("height", &height, 0.1f);
	ImGui::DragFloat("depth", &depth, 0.1f);
#endif // _DEBUGMODE
}

WorldShape AABBCollider::GetWorldShape() const {
	// キャッシュ済みの worldPos は InfoUpdate() を呼び忘れると古いままなので、その場で作り直す
	const Matrix4x4 world = GetWorldMatrix();

	WorldShape shape;
	shape.shape = ColliderShape::Box;
	shape.box.center = { world.m[3][0],world.m[3][1],world.m[3][2] };
	shape.box.size = { width / 2.0f, height / 2.0f, depth / 2.0f };
	// 回転（ワールド行列の基底をそのまま軸にする）
	shape.box.axes = {
		Vector3(world.m[0][0],world.m[0][1],world.m[0][2]).Normalize(),
		Vector3(world.m[1][0],world.m[1][1],world.m[1][2]).Normalize(),
		Vector3(world.m[2][0],world.m[2][1],world.m[2][2]).Normalize(),
	};
	shape.bounds = { shape.box.center, shape.box.size.Length() };
	return shape;
}

void AABBCollider::SaveCollider(const std::string& filePath) {
#ifdef _DEBUGMODE
	nlohmann::json json;
	WriteCommonJson(json);
	json["shape"] = "Box";
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
