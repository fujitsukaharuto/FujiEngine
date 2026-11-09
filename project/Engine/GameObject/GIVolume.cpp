#include "GIVolume.h"
#include <algorithm>
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

	void GIVolume::Create(const nlohmann::json& data) {
		SetName(data.value("name", std::string("GIVolume")));
		transform_.translate = ReadVector3(data, "center", { 0.0f,2.0f,0.0f });
		transform_.scale = ReadVector3(data, "size", { 10.0f,4.0f,10.0f });
		spacing_ = data.value("spacing", spacing_);
	}

	nlohmann::json GIVolume::ToJson() const {
		nlohmann::json data;
		data["name"] = GetName();
		data["center"] = { transform_.translate.x, transform_.translate.y, transform_.translate.z };
		data["size"] = { transform_.scale.x, transform_.scale.y, transform_.scale.z };
		data["spacing"] = spacing_;
		return data;
	}

	Vector3 GIVolume::GetSize() const {
		return { std::abs(transform_.scale.x), std::abs(transform_.scale.y), std::abs(transform_.scale.z) };
	}

	void GIVolume::Draw([[maybe_unused]] bool is) {
	#ifdef _DEBUGMODE
		// 箱の12本の辺
		const Vector4 color = { 1.0f,0.75f,0.2f,1.0f };
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
	#endif // _DEBUGMODE
	}

	void GIVolume::ParameterGUI() {
	#ifdef _DEBUGMODE
		char name[64] = {};
		GetName().copy(name, sizeof(name) - 1);
		if (ImGui::InputText("Name", name, sizeof(name))) {
			SetName(name);
		}
		ImGui::DragFloat("Probe spacing", &spacing_, 0.05f, 0.25f, 10.0f, "%.2f m");
		const Vector3 size = GetSize();
		ImGui::Text("%d x %d x %d",
			std::clamp(static_cast<int>(std::ceil(size.x / spacing_)) + 1, 2, 64),
			std::clamp(static_cast<int>(std::ceil(size.y / spacing_)) + 1, 2, 64),
			std::clamp(static_cast<int>(std::ceil(size.z / spacing_)) + 1, 2, 64));
	#endif // _DEBUGMODE
	}

}
