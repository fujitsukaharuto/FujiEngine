#include "PlacedObject.h"
#include <iterator>
#include <json.hpp>
#include "Engine/Core/Debug/ImGuiManager.h"

// 名前空間とクラス名が同名なので using namespace GameObject; は書けない
namespace GameObject {

	using namespace Graphics;
	using namespace Math;


	namespace {

		struct LightName {
			const char* name;
			LightMode mode;
		};
		// データには番号でなく名前で書く。enum の並びが変わっても読めるように
		constexpr LightName kLightNames[] = {
			{ "None",        LightMode::kLightNone },
			{ "HalfLambert", LightMode::kLightHalfLambert },
			{ "Lambert",     LightMode::kLightLambert },
			{ "Phong",       LightMode::kPhongReflect },
			{ "BlinnPhong",  LightMode::kBlinnPhongReflection },
			{ "PointLight",  LightMode::kPointLightON },
			{ "SpotLight",   LightMode::kSpotLightON },
		};
		// Material::CreateMaterial の既定と揃える
		constexpr LightMode kDefaultLight = LightMode::kPointLightON;

		std::optional<float> ReadFloat(const nlohmann::json& data, const char* key) {
			if (data.contains(key) && data[key].is_number()) {
				return data[key].get<float>();
			}
			return std::nullopt;
		}

		std::optional<Vector2> ReadVector2(const nlohmann::json& data, const char* key) {
			if (data.contains(key) && data[key].is_array() && data[key].size() == 2) {
				const auto& v = data[key];
				return Vector2{ v[0].get<float>(), v[1].get<float>() };
			}
			return std::nullopt;
		}

		std::optional<Vector3> ReadVector3(const nlohmann::json& data, const char* key) {
			if (data.contains(key) && data[key].is_array() && data[key].size() == 3) {
				const auto& v = data[key];
				return Vector3{ v[0].get<float>(), v[1].get<float>(), v[2].get<float>() };
			}
			return std::nullopt;
		}

		std::optional<Vector4> ReadVector4(const nlohmann::json& data, const char* key) {
			if (data.contains(key) && data[key].is_array() && data[key].size() == 4) {
				const auto& v = data[key];
				return Vector4{ v[0].get<float>(), v[1].get<float>(), v[2].get<float>(), v[3].get<float>() };
			}
			return std::nullopt;
		}

		std::optional<LightMode> ReadLight(const nlohmann::json& data, const char* key) {
			if (data.contains(key) && data[key].is_string()) {
				const std::string name = data[key].get<std::string>();
				for (const LightName& light : kLightNames) {
					if (name == light.name) {
						return light.mode;
					}
				}
			}
			return std::nullopt;
		}

		const char* LightToName(LightMode mode) {
			for (const LightName& light : kLightNames) {
				if (light.mode == mode) {
					return light.name;
				}
			}
			return kLightNames[0].name;
		}

	#ifdef _DEBUGMODE
		// 上書きするかをチェックボックスで切り替える。入れた瞬間は今の見た目(fallback)から始まるので絵が飛ばない。
		// 外した項目はモデルへ戻せないので、次に読み込んだ時からモデルの既定値になる
		template<class T, class EditFunc>
		bool OptionalGUI(const char* label, std::optional<T>& value, const T& fallback, EditFunc edit) {
			ImGui::PushID(label);
			bool isOn = value.has_value();
			bool changed = false;
			if (ImGui::Checkbox("##on", &isOn)) {
				value = isOn ? std::optional<T>(fallback) : std::nullopt;
				changed = true;
			}
			ImGui::SameLine();
			ImGui::BeginDisabled(!isOn);
			T temp = value.value_or(fallback);
			if (edit(label, temp) && value) {
				*value = temp;
				changed = true;
			}
			ImGui::EndDisabled();
			ImGui::PopID();
			return changed;
		}
	#endif // _DEBUGMODE
	}

	void PlacedObject::Create(const nlohmann::json& data) {
		look_.model = data.value("model", "");
		look_.isAnime = data.value("anime", false);
		look_.isAnimation = data.value("animation", false);
		look_.texture = data.value("texture", "");
		look_.color = ReadVector4(data, "color");
		look_.uvScale = ReadVector2(data, "uvScale");
		look_.roughness = ReadFloat(data, "roughness");
		look_.metallic = ReadFloat(data, "metallic");
		look_.environment = ReadFloat(data, "environment");
		look_.isMirror = data.value("mirror", false);
		look_.light = ReadLight(data, "light");

		SetName(data.value("name", look_.model));
		if (data.contains("transform")) {
			const auto& t = data["transform"];
			transform_.translate = ReadVector3(t, "translate").value_or(transform_.translate);
			transform_.rotate = ReadVector3(t, "rotate").value_or(transform_.rotate);
			transform_.scale = ReadVector3(t, "scale").value_or(transform_.scale);
		}
		Build();
	}

	void PlacedObject::Create(const std::string& modelName) {
		look_ = Look{};
		look_.model = modelName;
		SetName(modelName);
		Build();
	}

	nlohmann::json PlacedObject::ToJson() const {
		nlohmann::json data;
		data["name"] = GetName();
		data["model"] = look_.model;
		data["transform"] = {
			{ "translate", { transform_.translate.x, transform_.translate.y, transform_.translate.z } },
			{ "rotate",    { transform_.rotate.x,    transform_.rotate.y,    transform_.rotate.z } },
			{ "scale",     { transform_.scale.x,     transform_.scale.y,     transform_.scale.z } },
		};
		// 既定のままの項目は書かない。読み込み側で「書いてない＝モデルのまま」になる
		if (look_.isAnime) { data["anime"] = true; }
		if (look_.isAnimation) { data["animation"] = true; }
		if (!look_.texture.empty()) { data["texture"] = look_.texture; }
		if (look_.color) { data["color"] = { look_.color->x, look_.color->y, look_.color->z, look_.color->w }; }
		if (look_.uvScale) { data["uvScale"] = { look_.uvScale->x, look_.uvScale->y }; }
		if (look_.roughness) { data["roughness"] = *look_.roughness; }
		if (look_.metallic) { data["metallic"] = *look_.metallic; }
		if (look_.environment) { data["environment"] = *look_.environment; }
		if (look_.isMirror) { data["mirror"] = true; }
		if (look_.light) { data["light"] = LightToName(*look_.light); }
		return data;
	}

	void PlacedObject::Update() {
		if (animeModel_ && look_.isAnimation) {
			animeModel_->AnimationUpdate();
		}
		GameObject::Update();
	}

	void PlacedObject::Build() {
		if (look_.isAnime) {
			CreateAnimeModel(look_.model);
			if (look_.isAnimation) {
				animeModel_->LoadAnimationFile(look_.model);
			}
		} else {
			CreateModel(look_.model);
		}
		ApplyLook();
	}

	void PlacedObject::ApplyLook() {
		RenderObject* object = GetRenderObject();
		if (!object) {
			return;
		}
		if (!look_.texture.empty()) {
			object->SetTexture(look_.texture);
		}
		if (look_.color) {
			object->SetColor(*look_.color);
		}
		if (look_.uvScale) {
			object->SetUVScale(*look_.uvScale, { 0.0f,0.0f });
		}
		for (size_t i = 0; i < object->GetMaterialCount(); ++i) {
			Material& material = object->GetMaterial(i);
			if (look_.roughness) {
				material.SetRoughness(*look_.roughness);
			}
			if (look_.metallic) {
				material.SetMetallic(*look_.metallic);
			}
		}
		if (look_.environment) {
			if (animeModel_) {
				animeModel_->SetEnvironmentCoeff(*look_.environment);
			} else {
				for (size_t i = 0; i < object->GetMaterialCount(); ++i) {
					object->GetMaterial(i).SetEnvironment(*look_.environment);
				}
			}
		}
		if (animeModel_) {
			animeModel_->IsMirrorOBJ(look_.isMirror);
		}
		if (look_.light) {
			object->SetLightEnable(*look_.light);
		}
	}

	void PlacedObject::ParameterGUI() {
	#ifdef _DEBUGMODE
		char name[64] = {};
		GetName().copy(name, sizeof(name) - 1);
		if (ImGui::InputText("Name", name, sizeof(name))) {
			SetName(name);
		}
		ImGui::LabelText("Model", "%s", look_.model.c_str());

		RenderObject* object = GetRenderObject();
		if (!object) {
			return;
		}
		Material& material = object->GetMaterial();
		bool changed = false;

		char texture[128] = {};
		look_.texture.copy(texture, sizeof(texture) - 1);
		if (ImGui::InputText("Texture", texture, sizeof(texture), ImGuiInputTextFlags_EnterReturnsTrue)) {
			look_.texture = texture;
			changed = true;
		}
		changed |= OptionalGUI("Color", look_.color, material.GetColor(),
			[](const char* label, Vector4& v) { return ImGui::ColorEdit4(label, &v.x); });
		changed |= OptionalGUI("UV Scale", look_.uvScale, material.GetUVScale(),
			[](const char* label, Vector2& v) { return ImGui::DragFloat2(label, &v.x, 0.1f); });
		changed |= OptionalGUI("Roughness", look_.roughness, material.GetRoughness(),
			[](const char* label, float& v) { return ImGui::SliderFloat(label, &v, 0.0f, 1.0f); });
		changed |= OptionalGUI("Metallic", look_.metallic, material.GetMetallic(),
			[](const char* label, float& v) { return ImGui::SliderFloat(label, &v, 0.0f, 1.0f); });
		changed |= OptionalGUI("Environment", look_.environment, material.GetEnvironment(),
			[](const char* label, float& v) { return ImGui::SliderFloat(label, &v, 0.0f, 1.0f); });
		changed |= OptionalGUI("Light", look_.light, kDefaultLight,
			[](const char* label, LightMode& mode) {
				int index = 0;
				for (int i = 0; i < static_cast<int>(std::size(kLightNames)); ++i) {
					if (kLightNames[i].mode == mode) {
						index = i;
					}
				}
				const bool isChanged = ImGui::Combo(label, &index, [](void*, int i) { return kLightNames[i].name; }, nullptr, static_cast<int>(std::size(kLightNames)));
				mode = kLightNames[index].mode;
				return isChanged;
			});
		if (animeModel_) {
			changed |= ImGui::Checkbox("Mirror", &look_.isMirror);
		}

		if (changed) {
			ApplyLook();
		}
	#endif // _DEBUGMODE
	}

}
