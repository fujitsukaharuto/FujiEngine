#include "BaseCollider.h"
#include <algorithm>
#include <json.hpp>
#include "Engine/GameObject/GameObject.h"
#include "Engine/Collision/CollisionManager.h"
#include "Engine/Core/Debug/ImGuiManager.h"

using namespace Math;
using namespace Collision;


BaseCollider::BaseCollider() {
}

Matrix4x4 BaseCollider::GetWorldMatrix() const {
	Matrix4x4 worldMatrix = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, info.pos + offset_);

	if (parent_) {
		worldMatrix = Multiply(worldMatrix, Math::RemoveScale(parent_->GetWorldMat()));
	}
	return worldMatrix;
}

void BaseCollider::InfoUpdate() {
	const Matrix4x4 worldMatrix = GetWorldMatrix();
	info.worldPos = { worldMatrix.m[3][0],worldMatrix.m[3][1],worldMatrix.m[3][2] };
}

void BaseCollider::OnCollision(const ColliderInfo& other) {
	// onCollisionEvents_ は Enter/Stay/Exit の3つ分しかないので None は必ず弾く
	if (state_ == CollisionState::None) {
		return;
	}

	if (onCollisionEvents_[static_cast<int>(state_)]) {
		onCollisionEvents_[static_cast<int>(state_)](other);
	}
	if (state_ == CollisionState::CollisionExit) {
		state_ = CollisionState::None;
	}
}

bool BaseCollider::CanCollide(const BaseCollider& a, const BaseCollider& b) {
	return (a.collisionLayer_ & b.collisionMask_) != 0 && (b.collisionLayer_ & a.collisionMask_) != 0;
}

bool BaseCollider::IsHitting(const BaseCollider* other) const {
	return std::any_of(hitList_.begin(), hitList_.end(),
		[other](const HitRecord& record) { return record.other == other; });
}

void BaseCollider::AddHit(const BaseCollider* other, const ColliderInfo& otherInfo) {
	hitList_.push_back({ other, otherInfo });
}

void BaseCollider::RemoveHit(const BaseCollider* other) {
	std::erase_if(hitList_, [other](const HitRecord& record) { return record.other == other; });
}

std::vector<ColliderInfo> BaseCollider::RemoveHitsIfNot(const std::function<bool(const BaseCollider*)>& isAlive) {
	std::vector<ColliderInfo> lost;

	for (auto itr = hitList_.begin(); itr != hitList_.end();) {
		if (isAlive(itr->other)) {
			++itr;
		} else {
			lost.push_back(itr->info);
			itr = hitList_.erase(itr);
		}
	}
	return lost;
}

Vector3 BaseCollider::GetPos() {
	InfoUpdate();
	return info.worldPos;
}

Vector3 BaseCollider::GetWorldPos() {
	return GetPos();
}

GameObject::GameObject* BaseCollider::GetOwner() {
	return info.owner;
}

void BaseCollider::WriteCommonJson(nlohmann::json& json) const {
	json["position"] = { info.pos.x, info.pos.y, info.pos.z };
	json["offset"] = { offset_.x, offset_.y, offset_.z };
	json["layer"] = collisionLayer_;
	json["mask"] = collisionMask_;
}

void BaseCollider::LayerGUI() {
#ifdef _DEBUGMODE
	// GUI に並べるレイヤーの数
	constexpr uint32_t kShownLayers = 8;

	// 属するレイヤーは1つを選ぶ。複数に属させたいときはコードで SetCollisionLayer
	int layer = 0;
	while (layer < static_cast<int>(kLayerCount) - 1 && (collisionLayer_ & (1u << layer)) == 0) {
		layer++;
	}
	if (ImGui::BeginCombo("layer", CollisionManager::GetLayerName(static_cast<uint32_t>(layer)).c_str())) {
		for (uint32_t i = 0; i < kShownLayers; i++) {
			if (ImGui::Selectable(CollisionManager::GetLayerName(i).c_str(), static_cast<uint32_t>(layer) == i)) {
				collisionLayer_ = 1u << i;
			}
		}
		ImGui::EndCombo();
	}
	if (ImGui::TreeNode("mask")) {
		for (uint32_t i = 0; i < kShownLayers; i++) {
			bool isOn = (collisionMask_ & (1u << i)) != 0;
			if (ImGui::Checkbox(CollisionManager::GetLayerName(i).c_str(), &isOn)) {
				collisionMask_ = isOn ? (collisionMask_ | (1u << i)) : (collisionMask_ & ~(1u << i));
			}
		}
		ImGui::TreePop();
	}
#endif // _DEBUGMODE
}

void BaseCollider::DebugGUI() {
#ifdef _DEBUGMODE
	constexpr const char* kShapeNames[] = { "Box", "Sphere", "Capsule" };

	ImGui::Indent();
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Selected;
	if (ImGui::TreeNodeEx("Collider", flags, "Collider (%s)", kShapeNames[static_cast<int>(GetShape())])) {
		ImGui::DragFloat3("position", &info.pos.x, 0.1f);
		ImGui::DragFloat3("offset", &offset_.x, 0.1f);
		ShapeGUI();
		ImGui::Checkbox("isCollision", &isCollisionCheck_);
		LayerGUI();

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
#endif // _DEBUGMODE
}
