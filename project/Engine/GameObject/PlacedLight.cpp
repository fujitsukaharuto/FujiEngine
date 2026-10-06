#include "PlacedLight.h"
#include <algorithm>
#include <cmath>
#include <json.hpp>
#include "Engine/Graphics/Light/LightManager.h"
#include "Engine/Graphics/Line/Line3dDrawer.h"
#include "Engine/Core/Debug/ImGuiManager.h"

// 名前空間とクラス名が同名なので using namespace GameObject; は書けない
namespace GameObject {

	using namespace Math;


	namespace {
		constexpr float kDegToRad = 3.14159265f / 180.0f;

		Vector3 ReadVector3(const nlohmann::json& data, const char* key, const Vector3& fallback) {
			if (data.contains(key) && data[key].is_array() && data[key].size() == 3) {
				const auto& v = data[key];
				return { v[0].get<float>(), v[1].get<float>(), v[2].get<float>() };
			}
			return fallback;
		}
	}

	void PlacedLight::Create(const nlohmann::json& data, Graphics::LightManager* lightManager) {
		type_ = (data.value("type", "Point") == "Spot") ? Type::Spot : Type::Point;
		SetName(data.value("name", type_ == Type::Spot ? "SpotLight" : "PointLight"));
		transform_.translate = ReadVector3(data, "translate", transform_.translate);
		transform_.rotate = ReadVector3(data, "rotate", transform_.rotate);
		color_ = ReadVector3(data, "color", color_);
		intensity_ = data.value("intensity", intensity_);
		range_ = data.value("range", range_);
		decay_ = data.value("decay", decay_);
		angle_ = data.value("angle", angle_);
		softness_ = data.value("softness", softness_);
		Register(lightManager);
	}

	void PlacedLight::Create(Type type, Graphics::LightManager* lightManager) {
		type_ = type;
		SetName(type_ == Type::Spot ? "SpotLight" : "PointLight");
		// 原点だと地面に埋まって見えないので少し浮かせる。スポットライトは真下へ向ける
		transform_.translate = { 0.0f,3.0f,0.0f };
		if (type_ == Type::Spot) {
			transform_.rotate = { 90.0f * kDegToRad,0.0f,0.0f };
		}
		Register(lightManager);
	}

	nlohmann::json PlacedLight::ToJson() const {
		nlohmann::json data;
		data["name"] = GetName();
		data["type"] = (type_ == Type::Spot) ? "Spot" : "Point";
		data["translate"] = { transform_.translate.x, transform_.translate.y, transform_.translate.z };
		data["rotate"] = { transform_.rotate.x, transform_.rotate.y, transform_.rotate.z };
		data["color"] = { color_.x, color_.y, color_.z };
		data["intensity"] = intensity_;
		data["range"] = range_;
		data["decay"] = decay_;
		if (type_ == Type::Spot) {
			data["angle"] = angle_;
			data["softness"] = softness_;
		}
		return data;
	}

	void PlacedLight::Register(Graphics::LightManager* lightManager) {
		lightManager_ = lightManager;
		if (!lightManager_) {
			return;
		}
		slot_ = (type_ == Type::Spot)
			? lightManager_->AddSpotLight(Graphics::SpotLightData{})
			: lightManager_->AddPointLight(Graphics::PointLightData{});
		Apply(intensity_);
	}

	void PlacedLight::Update() {
		Apply(IsActive() ? intensity_ : 0.0f);
	}

	void PlacedLight::TurnOff() {
		Apply(0.0f);
	}

	void PlacedLight::Apply(float intensity) {
		if (!lightManager_ || slot_ < 0) {
			return;
		}
		const Vector4 color = { color_.x, color_.y, color_.z, 1.0f };
		if (type_ == Type::Spot) {
			Graphics::SpotLightData* light = lightManager_->GetSpotLight(slot_);
			light->color = color;
			light->position = transform_.GetWorldPos();
			light->direction = transform_.GetForward();
			light->intensity = intensity;
			light->distance = range_;
			light->decay = decay_;
			const float outer = std::clamp(angle_, 1.0f, 89.0f) * kDegToRad;
			// 内側と外側が同じ角度だと割り算が0になるので、わずかに差を残す
			const float inner = outer * (1.0f - std::clamp(softness_, 0.01f, 1.0f));
			light->cosAngle = std::cos(outer);
			light->cosFalloffStart = std::cos(inner);
		} else {
			Graphics::PointLightData* light = lightManager_->GetPointLight(slot_);
			light->color = color;
			light->position = transform_.GetWorldPos();
			light->intensity = intensity;
			light->radius = range_;
			light->decay = decay_;
		}
	}

	void PlacedLight::Draw([[maybe_unused]] bool is) {
	#ifdef _DEBUGMODE
		// 置いた場所が分かるよう、光の色の小さな球を出す。スポットライトは照らす向きも
		const Vector4 color = { color_.x, color_.y, color_.z, 1.0f };
		const Vector3 position = transform_.GetWorldPos();
		Graphics::Line3dDrawer* line = Graphics::Line3dDrawer::GetInstance();
		line->DrawSphereLine(position, 0.3f, color);
		if (type_ == Type::Spot) {
			line->DrawLine3d(position, position + transform_.GetForward() * (std::min)(range_, 3.0f), color);
		}
	#endif // _DEBUGMODE
	}

	void PlacedLight::ParameterGUI() {
	#ifdef _DEBUGMODE
		char name[64] = {};
		GetName().copy(name, sizeof(name) - 1);
		if (ImGui::InputText("Name", name, sizeof(name))) {
			SetName(name);
		}
		ImGui::LabelText("Type", "%s", type_ == Type::Spot ? "Spot" : "Point");
		if (slot_ < 0) {
			ImGui::TextDisabled("(ライトの数が上限)");
		}

		ImGui::ColorEdit3("Color", &color_.x);
		ImGui::DragFloat("Intensity", &intensity_, 0.05f, 0.0f, 1000.0f);
		ImGui::DragFloat("Range", &range_, 0.1f, 0.0f, 1000.0f);
		ImGui::DragFloat("Decay", &decay_, 0.01f, 0.0f, 10.0f);
		if (type_ == Type::Spot) {
			ImGui::SliderFloat("Angle", &angle_, 1.0f, 89.0f, "%.1f deg");
			ImGui::SliderFloat("Softness", &softness_, 0.0f, 1.0f);
		}
	#endif // _DEBUGMODE
	}

}
