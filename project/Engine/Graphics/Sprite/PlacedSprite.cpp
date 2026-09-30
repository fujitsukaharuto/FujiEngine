#include "PlacedSprite.h"
#include <cmath>
#include <json.hpp>
#include "Engine/Graphics/Sprite/Sprite.h"
#include "Engine/Graphics/Texture/TextureManager.h"
#include "Engine/Core/Debug/ImGuiManager.h"

using namespace Graphics;
using namespace Math;


namespace {

	Vector2 ReadVector2(const nlohmann::json& data, const char* key, const Vector2& fallback) {
		if (data.contains(key) && data[key].is_array() && data[key].size() == 2) {
			const auto& v = data[key];
			return { v[0].get<float>(), v[1].get<float>() };
		}
		return fallback;
	}

	Vector4 ReadVector4(const nlohmann::json& data, const char* key, const Vector4& fallback) {
		if (data.contains(key) && data[key].is_array() && data[key].size() == 4) {
			const auto& v = data[key];
			return { v[0].get<float>(), v[1].get<float>(), v[2].get<float>(), v[3].get<float>() };
		}
		return fallback;
	}
}


PlacedSprite::PlacedSprite() = default;

PlacedSprite::~PlacedSprite() = default;

void PlacedSprite::Create(const nlohmann::json& data) {
	name_ = data.value("name", "");
	texture_ = data.value("texture", "white2x2.png");
	isVisible_ = data.value("visible", true);
	Build();
	// 大きさを書いていなければテクスチャの大きさのまま
	size_ = ReadVector2(data, "size", sprite_->GetDefaultSize());
	pos_ = ReadVector2(data, "pos", pos_);
	anchor_ = ReadVector2(data, "anchor", anchor_);
	color_ = ReadVector4(data, "color", color_);
	Apply();
}

void PlacedSprite::Create(const std::string& texture) {
	texture_ = texture;
	Build();
	size_ = sprite_->GetDefaultSize();
	Apply();
}

nlohmann::json PlacedSprite::ToJson() const {
	nlohmann::json data;
	if (!name_.empty()) {
		data["name"] = name_;
	}
	data["texture"] = texture_;
	data["pos"] = { pos_.x, pos_.y };
	data["size"] = { size_.x, size_.y };
	data["anchor"] = { anchor_.x, anchor_.y };
	data["color"] = { color_.x, color_.y, color_.z, color_.w };
	data["visible"] = isVisible_;
	return data;
}

void PlacedSprite::Draw() {
	if (isVisible_) {
		sprite_->Draw();
	}
}

bool PlacedSprite::Contains(const Vector2& point) const {
	Vector2 min, max;
	GetRect(min, max);
	return point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y;
}

void PlacedSprite::GetRect(Vector2& min, Vector2& max) const {
	min = { pos_.x - size_.x * anchor_.x, pos_.y - size_.y * anchor_.y };
	max = { min.x + size_.x, min.y + size_.y };
}

void PlacedSprite::SetPos(const Vector2& pos) {
	pos_ = pos;
	sprite_->SetPos({ pos_.x, pos_.y, 0.0f });
}

void PlacedSprite::SetSize(const Vector2& size) {
	size_ = size;
	sprite_->SetSize(size_);
}

void PlacedSprite::SetColor(const Vector4& color) {
	color_ = color;
	sprite_->SetColor(color_);
}

void PlacedSprite::Build() {
	sprite_ = std::make_unique<Sprite>();
	sprite_->Load(texture_);
}

void PlacedSprite::Apply() {
	sprite_->SetAnchor(anchor_);
	sprite_->SetSize(size_);
	sprite_->SetPos({ pos_.x, pos_.y, 0.0f });
	sprite_->SetColor(color_);
}

void PlacedSprite::InspectorGUI() {
#ifdef _DEBUGMODE
	char name[64] = {};
	name_.copy(name, sizeof(name) - 1);
	if (ImGui::InputText("Name", name, sizeof(name))) {
		name_ = name;
	}

	if (ImGui::BeginCombo("Texture", texture_.c_str())) {
		for (const auto& file : TextureManager::GetInstance()->GetTextureFiles()) {
			if (ImGui::Selectable(file.first.c_str(), file.first == texture_)) {
				texture_ = file.first;
				Build();
				Apply();
			}
		}
		ImGui::EndCombo();
	}

	ImGui::Checkbox("Visible", &isVisible_);

	bool changed = false;
	changed |= ImGui::DragFloat2("Position", &pos_.x, 1.0f);
	changed |= ImGui::DragFloat2("Size", &size_.x, 1.0f);
	ImGui::SameLine();
	if (ImGui::SmallButton("元の大きさ")) {
		size_ = sprite_->GetDefaultSize();
		changed = true;
	}
	changed |= ImGui::SliderFloat2("Anchor", &anchor_.x, 0.0f, 1.0f);
	changed |= ImGui::ColorEdit4("Color", &color_.x);
	if (changed) {
		Apply();
	}
#endif // _DEBUGMODE
}
