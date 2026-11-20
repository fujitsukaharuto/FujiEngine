#include "CapsuleCollider.h"
#include <algorithm>
#include <cmath>
#include <json.hpp>
#include "Engine/Graphics/Line/Line3dDrawer.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/Core/Serialize/JsonSerializer.h"

using namespace Math;
using namespace Collision;

namespace {
	constexpr const char* kAxisNames[] = { "X", "Y", "Z" };
}


WorldShape CapsuleCollider::GetWorldShape() const {
	const Matrix4x4 world = GetWorldMatrix();
	const int row = static_cast<int>(axis_);
	const Vector3 center = { world.m[3][0],world.m[3][1],world.m[3][2] };
	const Vector3 axis = Vector3(world.m[row][0], world.m[row][1], world.m[row][2]).Normalize();
	const float radius = (std::max)(radius_, 0.0f);
	const float halfSegment = (std::max)(height_ * 0.5f - radius, 0.0f);

	WorldShape shape;
	shape.shape = ColliderShape::Capsule;
	shape.capsule = { center - axis * halfSegment, center + axis * halfSegment, radius };
	shape.bounds = { center, halfSegment + radius };
	return shape;
}

void CapsuleCollider::ShapeGUI() {
#ifdef _DEBUGMODE
	ImGui::DragFloat("radius", &radius_, 0.05f, 0.0f, 1000.0f);
	ImGui::DragFloat("height", &height_, 0.05f, 0.0f, 1000.0f);
	int axis = static_cast<int>(axis_);
	if (ImGui::Combo("axis", &axis, kAxisNames, IM_ARRAYSIZE(kAxisNames))) {
		axis_ = static_cast<Axis>(axis);
	}
#endif // _DEBUGMODE
}

void CapsuleCollider::SaveCollider(const std::string& filePath) {
#ifdef _DEBUGMODE
	nlohmann::json json;
	WriteCommonJson(json);
	json["shape"] = "Capsule";
	json["radius"] = radius_;
	json["height"] = height_;
	json["axis"] = kAxisNames[static_cast<int>(axis_)];

	nlohmann::json finalJson;
	finalJson["collider"] = json;
	std::filesystem::create_directories(kDirectoryPath_);
	Core::JsonSerializer::SerializeJsonData(finalJson, (std::filesystem::path(kDirectoryPath_) / filePath).string());
#endif // _DEBUGMODE
}

#ifdef _DEBUGMODE
void CapsuleCollider::DrawCollider() {
	const Capsule capsule = GetWorldShape().capsule;
	const Vector4 color = { 1.0f,1.0f,1.0f,1.0f };
	Graphics::Line3dDrawer* line = Graphics::Line3dDrawer::GetInstance();
	line->DrawSphereLine(capsule.a, capsule.radius, color);
	line->DrawSphereLine(capsule.b, capsule.radius, color);

	// 胴の4本の線。軸に直交する2方向
	const Vector3 axis = (capsule.b - capsule.a).Length() > 1e-6f ? (capsule.b - capsule.a).Normalize() : Vector3{ 0.0f,1.0f,0.0f };
	const Vector3 helper = std::abs(axis.y) < 0.99f ? Vector3{ 0.0f,1.0f,0.0f } : Vector3{ 1.0f,0.0f,0.0f };
	const Vector3 side = Vector3::Cross(axis, helper).Normalize() * capsule.radius;
	const Vector3 side2 = Vector3::Cross(axis, side).Normalize() * capsule.radius;
	for (const Vector3& offset : { side, side * -1.0f, side2, side2 * -1.0f }) {
		line->DrawLine3d(capsule.a + offset, capsule.b + offset, color);
	}
}
#endif // _DEBUGMODE
