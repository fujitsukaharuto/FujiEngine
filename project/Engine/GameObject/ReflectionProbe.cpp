#include "ReflectionProbe.h"
#include <cmath>
#include <json.hpp>
#include "Engine/Graphics/Line/Line3dDrawer.h"
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
	}

	void ReflectionProbe::Create(const nlohmann::json& data) {
		SetName(data.value("name", std::string("ReflectionProbe")));
		transform_.translate = ReadVector3(data, "center", { 0.0f,2.0f,0.0f });
		transform_.scale = ReadVector3(data, "size", { 10.0f,4.0f,10.0f });
		blendDistance_ = data.value("blendDistance", blendDistance_);
		intensity_ = data.value("intensity", intensity_);
		isBoxProjection_ = data.value("boxProjection", isBoxProjection_);
	}

	nlohmann::json ReflectionProbe::ToJson() const {
		nlohmann::json data;
		data["name"] = GetName();
		data["center"] = { transform_.translate.x, transform_.translate.y, transform_.translate.z };
		data["size"] = { transform_.scale.x, transform_.scale.y, transform_.scale.z };
		data["blendDistance"] = blendDistance_;
		data["intensity"] = intensity_;
		data["boxProjection"] = isBoxProjection_;
		return data;
	}

	Vector3 ReflectionProbe::GetSize() const {
		return { std::abs(transform_.scale.x), std::abs(transform_.scale.y), std::abs(transform_.scale.z) };
	}

	void ReflectionProbe::Draw([[maybe_unused]] bool is) {
	#ifdef _DEBUGMODE
		// 箱の12本の辺と、焼く位置の十字
		const Vector4 color = { 0.3f,0.8f,1.0f,1.0f };
		const Vector3 c = GetCenter();
		const Vector3 h = GetSize() * 0.5f;
		Vector3 corners[8];
		for (int i = 0; i < 8; i++) {
			corners[i] = { c.x + ((i & 1) ? h.x : -h.x), c.y + ((i & 2) ? h.y : -h.y), c.z + ((i & 4) ? h.z : -h.z) };
		}
		Graphics::Line3dDrawer* line = Graphics::Line3dDrawer::GetInstance();
		for (int i = 0; i < 8; i++) {
			for (int bit = 1; bit < 8; bit <<= 1) {
				if ((i & bit) == 0) {
					line->DrawLine3d(corners[i], corners[i | bit], color);
				}
			}
		}
		const float cross = 0.5f;
		line->DrawLine3d({ c.x - cross, c.y, c.z }, { c.x + cross, c.y, c.z }, color);
		line->DrawLine3d({ c.x, c.y - cross, c.z }, { c.x, c.y + cross, c.z }, color);
		line->DrawLine3d({ c.x, c.y, c.z - cross }, { c.x, c.y, c.z + cross }, color);
	#endif // _DEBUGMODE
	}

	void ReflectionProbe::ParameterGUI() {
	#ifdef _DEBUGMODE
		char name[64] = {};
		GetName().copy(name, sizeof(name) - 1);
		if (ImGui::InputText("Name", name, sizeof(name))) {
			SetName(name);
		}
		ImGui::DragFloat("Blend distance", &blendDistance_, 0.05f, 0.01f, 50.0f, "%.2f m");
		ImGui::DragFloat("Intensity", &intensity_, 0.01f, 0.0f, 4.0f);
		ImGui::Checkbox("Box projection", &isBoxProjection_);
	#endif // _DEBUGMODE
	}

}
