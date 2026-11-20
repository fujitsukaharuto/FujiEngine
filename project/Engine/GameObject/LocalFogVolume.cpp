#include "LocalFogVolume.h"
#include <cmath>
#include <numbers>
#include <json.hpp>
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

	void LocalFogVolume::Create(const nlohmann::json& data) {
		const Graphics::VolumetricFogPass::LocalFogVolumeDesc defaults;
		SetName(data.value("name", std::string("LocalFogVolume")));
		transform_.translate = ReadVector3(data, "center", { 0.0f,2.0f,0.0f });
		transform_.scale = ReadVector3(data, "radii", defaults.radii);
		transform_.rotate = ReadVector3(data, "rotation", { 0.0f,0.0f,0.0f });
		settings_.radialExtinction = data.value("radialExtinction", defaults.radialExtinction);
		settings_.heightExtinction = data.value("heightExtinction", defaults.heightExtinction);
		settings_.heightFalloff = data.value("heightFalloff", defaults.heightFalloff);
		settings_.heightOffset = data.value("heightOffset", defaults.heightOffset);
		settings_.albedo = ReadVector3(data, "albedo", defaults.albedo);
		settings_.emissive = ReadVector3(data, "emissive", defaults.emissive);
		settings_.phaseG = data.value("phaseG", defaults.phaseG);
	}

	nlohmann::json LocalFogVolume::ToJson() const {
		nlohmann::json data;
		data["name"] = GetName();
		data["center"] = ToArray(transform_.translate);
		data["radii"] = ToArray(transform_.scale);
		data["rotation"] = ToArray(transform_.rotate);
		data["radialExtinction"] = settings_.radialExtinction;
		data["heightExtinction"] = settings_.heightExtinction;
		data["heightFalloff"] = settings_.heightFalloff;
		data["heightOffset"] = settings_.heightOffset;
		data["albedo"] = ToArray(settings_.albedo);
		data["emissive"] = ToArray(settings_.emissive);
		data["phaseG"] = settings_.phaseG;
		return data;
	}

	Graphics::VolumetricFogPass::LocalFogVolumeDesc LocalFogVolume::GetDesc() const {
		Graphics::VolumetricFogPass::LocalFogVolumeDesc desc = settings_;
		desc.center = transform_.translate;
		desc.radii = { std::abs(transform_.scale.x), std::abs(transform_.scale.y), std::abs(transform_.scale.z) };
		desc.rotation = transform_.rotate;
		return desc;
	}

	void LocalFogVolume::Draw([[maybe_unused]] bool is) {
	#ifdef _DEBUGMODE
		// 楕円体の3つの断面の輪
		const Vector4 color = { 0.75f,0.8f,0.9f,1.0f };
		const Graphics::VolumetricFogPass::LocalFogVolumeDesc desc = GetDesc();
		const Matrix4x4 world = MakeAffineMatrix(desc.radii, desc.rotation, desc.center);
		Graphics::Line3dDrawer* line = Graphics::Line3dDrawer::GetInstance();
		constexpr int kSegments = 32;
		constexpr float kStep = 2.0f * std::numbers::pi_v<float> / static_cast<float>(kSegments);
		for (int axis = 0; axis < 3; axis++) {
			Vector3 previous{};
			for (int i = 0; i <= kSegments; i++) {
				const float c = std::cos(kStep * static_cast<float>(i));
				const float s = std::sin(kStep * static_cast<float>(i));
				const Vector3 local = (axis == 0) ? Vector3{ 0.0f, c, s } : (axis == 1) ? Vector3{ c, 0.0f, s } : Vector3{ c, s, 0.0f };
				const Vector3 point = Transform(local, world);
				if (i > 0) {
					line->DrawLine3d(previous, point, color);
				}
				previous = point;
			}
		}
	#endif // _DEBUGMODE
	}

	void LocalFogVolume::ParameterGUI() {
	#ifdef _DEBUGMODE
		char name[64] = {};
		GetName().copy(name, sizeof(name) - 1);
		if (ImGui::InputText("Name", name, sizeof(name))) {
			SetName(name);
		}
		ImGui::DragFloat("Radial extinction", &settings_.radialExtinction, 0.005f, 0.0f, 10.0f, "%.3f /m");
		ImGui::DragFloat("Height extinction", &settings_.heightExtinction, 0.005f, 0.0f, 10.0f, "%.3f /m");
		ImGui::BeginDisabled(settings_.heightExtinction <= 0.0f);
		ImGui::DragFloat("Height falloff", &settings_.heightFalloff, 0.01f, 0.0f, 10.0f, "%.3f /m");
		ImGui::DragFloat("Height offset", &settings_.heightOffset, 0.05f, -100.0f, 100.0f, "%.2f m");
		ImGui::EndDisabled();
		ImGui::ColorEdit3("Albedo", &settings_.albedo.x, ImGuiColorEditFlags_Float);
		ImGui::ColorEdit3("Emissive", &settings_.emissive.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
		ImGui::SliderFloat("Phase g", &settings_.phaseG, -0.9f, 0.9f);
	#endif // _DEBUGMODE
	}

}
