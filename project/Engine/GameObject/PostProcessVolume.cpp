#include "PostProcessVolume.h"
#include <algorithm>
#include <cmath>
#include <json.hpp>
#include "Engine/Scene/EnvironmentParams.h"
#include "Engine/Graphics/Line/Line3dDrawer.h"
#include "Engine/Math/Matrix/MatrixCalculation.h"
#include "Engine/Core/Debug/ImGuiManager.h"

// 名前空間とクラス名が同名なので using namespace は書けない
namespace GameObject {

	using namespace Math;


	namespace {
		Vector3 ReadVector3(const nlohmann::json& data, const char* key, const Vector3& fallback) {
			if (data.contains(key) && data[key].is_array() && data[key].size() == 3) {
				const auto& v = data[key];
				return { v[0].get<float>(), v[1].get<float>(), v[2].get<float>() };
			}
			return fallback;
		}

		nlohmann::json ToArray(const Vector3& v) {
			return { v.x, v.y, v.z };
		}
	}

	void PostProcessVolume::Create(const nlohmann::json& data) {
		SetName(data.value("name", std::string("PostProcessVolume")));
		transform_.translate = ReadVector3(data, "center", { 0.0f,2.0f,0.0f });
		transform_.scale = ReadVector3(data, "size", { 10.0f,4.0f,10.0f });
		transform_.rotate = ReadVector3(data, "rotation", { 0.0f,0.0f,0.0f });
		priority_ = data.value("priority", priority_);
		blendRadius_ = data.value("blendRadius", blendRadius_);
		blendWeight_ = data.value("blendWeight", blendWeight_);
		isUnbound_ = data.value("unbound", isUnbound_);
		overrides_.clear();
		if (data.contains("overrides") && data["overrides"].is_object()) {
			for (const auto& [key, value] : data["overrides"].items()) {
				const Scene::EnvironmentParam* param = Scene::FindVolumeParam(key);
				if (param == nullptr) {
					continue;
				}
				std::array<float, 3> values{};
				if (value.is_number()) {
					values[0] = value.get<float>();
				} else if (value.is_array()) {
					for (size_t i = 0; i < value.size() && i < values.size(); ++i) {
						values[i] = value[i].get<float>();
					}
				} else {
					continue;
				}
				overrides_[key] = values;
			}
		}
	}

	nlohmann::json PostProcessVolume::ToJson() const {
		nlohmann::json data;
		data["name"] = GetName();
		data["center"] = ToArray(transform_.translate);
		data["size"] = ToArray(transform_.scale);
		data["rotation"] = ToArray(transform_.rotate);
		data["priority"] = priority_;
		data["blendRadius"] = blendRadius_;
		data["blendWeight"] = blendWeight_;
		data["unbound"] = isUnbound_;
		nlohmann::json overrides = nlohmann::json::object();
		for (const auto& [key, values] : overrides_) {
			const Scene::EnvironmentParam* param = Scene::FindVolumeParam(key);
			if (param == nullptr) {
				continue;
			}
			if (param->count == 1) {
				overrides[key] = values[0];
			} else {
				overrides[key] = { values[0], values[1], values[2] };
			}
		}
		data["overrides"] = std::move(overrides);
		return data;
	}

	float PostProcessVolume::DistanceToBox(const Vector3& point) const {
		// 回転した箱の中の座標へ。大きさは掛けずに測るので距離は m のまま
		const Matrix4x4 toLocal = Inverse(MakeAffineMatrix({ 1.0f,1.0f,1.0f }, transform_.rotate, transform_.translate));
		const Vector3 local = Transform(point, toLocal);
		const float dx = (std::max)(std::abs(local.x) - std::abs(transform_.scale.x) * 0.5f, 0.0f);
		const float dy = (std::max)(std::abs(local.y) - std::abs(transform_.scale.y) * 0.5f, 0.0f);
		const float dz = (std::max)(std::abs(local.z) - std::abs(transform_.scale.z) * 0.5f, 0.0f);
		return std::sqrt(dx * dx + dy * dy + dz * dz);
	}

	float PostProcessVolume::GetWeight(const Vector3& cameraPosition) const {
		if (!IsActive() || overrides_.empty()) {
			return 0.0f;
		}
		// UE の DoPostProcessVolume と同じ。中なら Blend weight、外は Blend radius まで距離に比例して薄める
		float weight = std::clamp(blendWeight_, 0.0f, 1.0f);
		if (isUnbound_) {
			return weight;
		}
		const float distance = DistanceToBox(cameraPosition);
		if (distance > blendRadius_) {
			return 0.0f;
		}
		if (blendRadius_ >= 0.01f) {
			weight *= 1.0f - distance / blendRadius_;
		}
		return weight;
	}

	void PostProcessVolume::Blend(Scene::Environment& env, float weight) const {
		for (const auto& [key, values] : overrides_) {
			const Scene::EnvironmentParam* param = Scene::FindVolumeParam(key);
			if (param == nullptr) {
				continue;
			}
			float* target = param->get(env);
			for (int i = 0; i < param->count; ++i) {
				target[i] += (values[i] - target[i]) * weight;
			}
		}
	}

	void PostProcessVolume::Draw([[maybe_unused]] bool is) {
	#ifdef _DEBUGMODE
		if (isUnbound_) {
			return;
		}
		const Vector4 color = { 0.9f,0.55f,1.0f,1.0f };
		const Matrix4x4 world = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
		Vector3 corners[8];
		for (int i = 0; i < 8; i++) {
			const Vector3 local = { (i & 1) ? 0.5f : -0.5f, (i & 2) ? 0.5f : -0.5f, (i & 4) ? 0.5f : -0.5f };
			corners[i] = Transform(local, world);
		}
		Graphics::Line3dDrawer* line = Graphics::Line3dDrawer::GetInstance();
		for (int i = 0; i < 8; i++) {
			for (int bit = 1; bit < 8; bit <<= 1) {
				if ((i & bit) == 0) {
					line->DrawLine3d(corners[i], corners[i | bit], color);
				}
			}
		}
	#endif // _DEBUGMODE
	}

	void PostProcessVolume::ParameterGUI() {
	#ifdef _DEBUGMODE
		char name[64] = {};
		GetName().copy(name, sizeof(name) - 1);
		if (ImGui::InputText("Name", name, sizeof(name))) {
			SetName(name);
		}
		ImGui::DragInt("Priority", &priority_);
		ImGui::Checkbox("Unbound", &isUnbound_);
		ImGui::BeginDisabled(isUnbound_);
		ImGui::DragFloat("Blend radius", &blendRadius_, 0.05f, 0.0f, 100.0f, "%.2f m");
		ImGui::EndDisabled();
		ImGui::SliderFloat("Blend weight", &blendWeight_, 0.0f, 1.0f);

		ImGui::SeparatorText("Overrides");
		const Scene::Environment defaults;
		for (const Scene::EnvironmentParam& param : Scene::GetVolumeParams()) {
			ImGui::PushID(param.name);
			auto it = overrides_.find(param.name);
			bool isOn = it != overrides_.end();
			if (ImGui::Checkbox("##on", &isOn)) {
				if (isOn) {
					// 既定の環境の値から始める
					std::array<float, 3> values{};
					Scene::Environment source = defaults;
					const float* initial = param.get(source);
					for (int i = 0; i < param.count; ++i) {
						values[i] = initial[i];
					}
					it = overrides_.emplace(param.name, values).first;
				} else {
					overrides_.erase(it);
					it = overrides_.end();
				}
			}
			ImGui::SameLine();
			ImGui::BeginDisabled(!isOn);
			std::array<float, 3> dummy{};
			float* values = isOn ? it->second.data() : dummy.data();
			if (param.count == 3 && param.isColor) {
				ImGui::ColorEdit3(param.name, values, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
			} else if (param.count == 3) {
				ImGui::DragFloat3(param.name, values, param.speed, param.minValue, param.maxValue);
			} else {
				ImGui::DragFloat(param.name, values, param.speed, param.minValue, param.maxValue);
			}
			ImGui::EndDisabled();
			ImGui::PopID();
		}
	#endif // _DEBUGMODE
	}

}
