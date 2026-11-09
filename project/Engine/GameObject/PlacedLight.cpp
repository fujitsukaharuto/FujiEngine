#include "PlacedLight.h"
#include <algorithm>
#include <cmath>
#include <json.hpp>
#include "Engine/Graphics/Light/LightManager.h"
#include "Engine/Core/Time/FPSKeeper.h"
#include "Engine/Graphics/Line/Line3dDrawer.h"
#include "Engine/Core/Debug/ImGuiManager.h"

// 名前空間とクラス名が同名なので using namespace GameObject; は書けない
namespace GameObject {

	using namespace Math;


	namespace {
		constexpr float kDegToRad = 3.14159265f / 180.0f;

		constexpr const char* kTypeNames[] = { "Point", "Spot", "Tube", "Rect" };

		PlacedLight::Type TypeFromName(const std::string& name) {
			for (int i = 0; i < static_cast<int>(std::size(kTypeNames)); i++) {
				if (name == kTypeNames[i]) {
					return static_cast<PlacedLight::Type>(i);
				}
			}
			return PlacedLight::Type::Point;
		}

		std::string DefaultName(PlacedLight::Type type) {
			return std::string(kTypeNames[static_cast<int>(type)]) + "Light";
		}

		Vector3 ReadVector3(const nlohmann::json& data, const char* key, const Vector3& fallback) {
			if (data.contains(key) && data[key].is_array() && data[key].size() == 3) {
				const auto& v = data[key];
				return { v[0].get<float>(), v[1].get<float>(), v[2].get<float>() };
			}
			return fallback;
		}
	}

	void PlacedLight::Create(const nlohmann::json& data, Graphics::LightManager* lightManager) {
		type_ = TypeFromName(data.value("type", std::string("Point")));
		SetName(data.value("name", DefaultName(type_)));
		transform_.translate = ReadVector3(data, "translate", transform_.translate);
		transform_.rotate = ReadVector3(data, "rotate", transform_.rotate);
		color_ = ReadVector3(data, "color", color_);
		intensity_ = data.value("intensity", intensity_);
		range_ = data.value("range", range_);
		decay_ = data.value("decay", decay_);
		angle_ = data.value("angle", angle_);
		softness_ = data.value("softness", softness_);
		castShadow_ = data.value("shadow", castShadow_);
		width_ = data.value("width", width_);
		height_ = data.value("height", height_);
		isTwoSided_ = data.value("twoSided", isTwoSided_);
		flicker_ = EmissiveFlicker::Read(data);
		Register(lightManager);
	}

	void PlacedLight::Create(Type type, Graphics::LightManager* lightManager) {
		type_ = type;
		SetName(DefaultName(type_));
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
		data["type"] = kTypeNames[static_cast<int>(type_)];
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
		if (IsShaped()) {
			data["width"] = width_;
			if (type_ == Type::Rect) {
				data["height"] = height_;
				if (isTwoSided_) {
					data["twoSided"] = true;
				}
			}
		}
		flicker_.Write(data);
		// 外したときだけ書く
		if (!castShadow_ && !IsShaped()) {
			data["shadow"] = false;
		}
		return data;
	}

	void PlacedLight::Register(Graphics::LightManager* lightManager) {
		lightManager_ = lightManager;
		if (!lightManager_) {
			return;
		}
		// 形のある光源は影なしの枠にしか置けない
		isClustered_ = !castShadow_ || IsShaped();
		if (isClustered_) {
			slot_ = lightManager_->AddClusteredLight();
		} else {
			slot_ = (type_ == Type::Spot)
				? lightManager_->AddSpotLight(Graphics::SpotLightData{})
				: lightManager_->AddPointLight(Graphics::PointLightData{});
		}
		Apply(intensity_);
	}

	void PlacedLight::Unregister() {
		Apply(0.0f);
		if (lightManager_ && isClustered_) {
			lightManager_->RemoveClusteredLight(slot_);
		}
		slot_ = -1;
	}

	void PlacedLight::Update() {
		time_ += Core::FPSKeeper::DeltaTime();
		const float flicker = flicker_.IsEnabled() ? flicker_.Evaluate(time_) : 1.0f;
		Apply(IsActive() ? intensity_ * flicker : 0.0f);
	}

	void PlacedLight::TurnOff() {
		Unregister();
	}

	void PlacedLight::Apply(float intensity) {
		if (!lightManager_ || slot_ < 0) {
			return;
		}
		const float outer = std::clamp(angle_, 1.0f, 89.0f) * kDegToRad;
		// 内側と外側が同じ角度にならないよう差を残す
		const float inner = outer * (1.0f - std::clamp(softness_, 0.01f, 1.0f));
		if (isClustered_) {
			Graphics::ClusteredLightData* light = lightManager_->GetClusteredLight(slot_);
			if (!light) {
				return;
			}
			light->position = transform_.GetWorldPos();
			light->color = color_ * intensity;
			light->range = range_;
			light->decay = decay_;
			light->pad[0] = 0.0f;
			light->pad[1] = 0.0f;
			if (type_ == Type::Tube) {
				light->type = Graphics::kClusteredLightTube;
				light->direction = transform_.GetRight() * ((std::max)(width_, 0.01f) * 0.5f);
				return;
			}
			if (type_ == Type::Rect) {
				// 前(+Z)を照らす。縦の軸はスポット用の欄へ
				const Vector3 v = transform_.GetUp() * ((std::max)(height_, 0.01f) * 0.5f);
				light->type = Graphics::kClusteredLightRect;
				light->direction = transform_.GetRight() * ((std::max)(width_, 0.01f) * 0.5f);
				light->cosAngle = v.x;
				light->cosFalloffStart = v.y;
				light->pad[0] = v.z;
				light->pad[1] = isTwoSided_ ? 1.0f : 0.0f;
				return;
			}
			light->type = (type_ == Type::Spot) ? Graphics::kClusteredLightSpot : Graphics::kClusteredLightPoint;
			light->direction = transform_.GetForward().Normalize();
			light->cosAngle = std::cos(outer);
			light->cosFalloffStart = std::cos(inner);
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
		if (type_ == Type::Tube) {
			const Vector3 half = transform_.GetRight() * (width_ * 0.5f);
			line->DrawLine3d(position - half, position + half, color);
			return;
		}
		if (type_ == Type::Rect) {
			const Vector3 u = transform_.GetRight() * (width_ * 0.5f);
			const Vector3 v = transform_.GetUp() * (height_ * 0.5f);
			const Vector3 corners[4] = { position - u - v, position + u - v, position + u + v, position - u + v };
			for (int i = 0; i < 4; i++) {
				line->DrawLine3d(corners[i], corners[(i + 1) % 4], color);
			}
			// 照らす向き
			line->DrawLine3d(position, position + transform_.GetForward() * 0.5f, color);
			return;
		}
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
		ImGui::LabelText("Type", "%s", kTypeNames[static_cast<int>(type_)]);
		if (!IsShaped() && ImGui::Checkbox("Shadow", &castShadow_)) {
			// 枠の種類が変わるので取り直す
			Graphics::LightManager* lightManager = lightManager_;
			Unregister();
			Register(lightManager);
		}
		if (slot_ < 0) {
			ImGui::TextDisabled("(ライトの数が上限)");
		}

		ImGui::ColorEdit3("Color", &color_.x);
		ImGui::DragFloat("Intensity", &intensity_, 0.05f, 0.0f, 1000.0f);
		ImGui::DragFloat("Range", &range_, 0.1f, 0.0f, 1000.0f);
		ImGui::DragFloat("Decay", &decay_, 0.01f, 0.0f, 10.0f);
		if (IsShaped()) {
			ImGui::DragFloat(type_ == Type::Tube ? "Length" : "Width", &width_, 0.01f, 0.01f, 100.0f, "%.2f m");
		}
		if (type_ == Type::Rect) {
			ImGui::DragFloat("Height", &height_, 0.01f, 0.01f, 100.0f, "%.2f m");
			ImGui::Checkbox("Two sided", &isTwoSided_);
		}
		flicker_.GUI();
		if (type_ == Type::Spot) {
			ImGui::SliderFloat("Angle", &angle_, 1.0f, 89.0f, "%.1f deg");
			ImGui::SliderFloat("Softness", &softness_, 0.0f, 1.0f);
		}
	#endif // _DEBUGMODE
	}

}
