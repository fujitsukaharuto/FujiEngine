#include "SpawnPoint.h"
#include <json.hpp>
#include "Engine/Graphics/Line/Line3dDrawer.h"
#include "Engine/Core/Debug/ImGuiManager.h"

// 名前空間とクラス名が同名なので using namespace GameObject; は書けない
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

	void SpawnPoint::Create(const nlohmann::json& data) {
		SetName(data.value("name", ""));
		transform_.translate = ReadVector3(data, "translate", transform_.translate);
		transform_.rotate = ReadVector3(data, "rotate", transform_.rotate);
	}

	nlohmann::json SpawnPoint::ToJson() const {
		nlohmann::json data;
		data["name"] = GetName();
		data["translate"] = { transform_.translate.x, transform_.translate.y, transform_.translate.z };
		data["rotate"] = { transform_.rotate.x, transform_.rotate.y, transform_.rotate.z };
		return data;
	}

	void SpawnPoint::Draw([[maybe_unused]] bool is) {
	#ifdef _DEBUGMODE
		// 足元の輪と、立てた棒の上から向きの矢印
		const Vector4 color = { 0.2f,0.9f,0.4f,1.0f };
		const Vector3 base = transform_.GetWorldPos();
		const Vector3 top = base + Vector3{ 0.0f,2.0f,0.0f };
		const Vector3 tip = top + transform_.GetForward() * 1.5f;
		Graphics::Line3dDrawer* line = Graphics::Line3dDrawer::GetInstance();
		line->DrawSphereLine(base, 0.5f, color);
		line->DrawLine3d(base, top, color);
		line->DrawLine3d(top, tip, color);
	#endif // _DEBUGMODE
	}

	void SpawnPoint::ParameterGUI() {
	#ifdef _DEBUGMODE
		char name[64] = {};
		GetName().copy(name, sizeof(name) - 1);
		if (ImGui::InputText("Name", name, sizeof(name))) {
			SetName(name);
		}
	#endif // _DEBUGMODE
	}

}
