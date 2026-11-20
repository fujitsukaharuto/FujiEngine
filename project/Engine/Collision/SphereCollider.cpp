#include "SphereCollider.h"
#include <algorithm>
#include <json.hpp>
#include "Engine/Graphics/Line/Line3dDrawer.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/Core/Serialize/JsonSerializer.h"

using namespace Math;
using namespace Collision;


WorldShape SphereCollider::GetWorldShape() const {
	const Matrix4x4 world = GetWorldMatrix();
	WorldShape shape;
	shape.shape = ColliderShape::Sphere;
	shape.sphere = { { world.m[3][0],world.m[3][1],world.m[3][2] }, (std::max)(radius_, 0.0f) };
	shape.bounds = shape.sphere;
	return shape;
}

void SphereCollider::ShapeGUI() {
#ifdef _DEBUGMODE
	ImGui::DragFloat("radius", &radius_, 0.05f, 0.0f, 1000.0f);
#endif // _DEBUGMODE
}

void SphereCollider::SaveCollider(const std::string& filePath) {
#ifdef _DEBUGMODE
	nlohmann::json json;
	WriteCommonJson(json);
	json["shape"] = "Sphere";
	json["radius"] = radius_;

	nlohmann::json finalJson;
	finalJson["collider"] = json;
	std::filesystem::create_directories(kDirectoryPath_);
	Core::JsonSerializer::SerializeJsonData(finalJson, (std::filesystem::path(kDirectoryPath_) / filePath).string());
#endif // _DEBUGMODE
}

#ifdef _DEBUGMODE
void SphereCollider::DrawCollider() {
	const WorldShape shape = GetWorldShape();
	Graphics::Line3dDrawer::GetInstance()->DrawSphereLine(shape.sphere.center, shape.sphere.radius, { 1.0f,1.0f,1.0f,1.0f });
}
#endif // _DEBUGMODE
