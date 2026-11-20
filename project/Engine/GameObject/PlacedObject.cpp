#include "PlacedObject.h"
#include <algorithm>
#include <cmath>
#include <iterator>
#include <numbers>
#include <json.hpp>
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/Graphics/Light/LightManager.h"
#include "Engine/Graphics/Object/ObjectRenderer.h"
#include "Engine/Graphics/Particle/RainRenderer.h"
#include "Engine/Core/Time/FPSKeeper.h"
#include "Engine/Graphics/Model/Model.h"

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
		look_.emissiveTexture = data.value("emissiveTexture", "");
		look_.color = ReadVector4(data, "color");
		look_.uvScale = ReadVector2(data, "uvScale");
		look_.roughness = ReadFloat(data, "roughness");
		look_.metallic = ReadFloat(data, "metallic");
		look_.environment = ReadFloat(data, "environment");
		look_.weathering = ReadFloat(data, "weathering");
		look_.flowWater = ReadFloat(data, "flowWater");
		look_.clearCoat = ReadFloat(data, "clearCoat");
		look_.clearCoatRoughness = data.value("clearCoatRoughness", 0.05f);
		look_.anisotropy = ReadFloat(data, "anisotropy");
		look_.anisotropyRotation = data.value("anisotropyRotation", 0.0f);
		look_.sheen = ReadVector3(data, "sheen");
		look_.sheenRoughness = data.value("sheenRoughness", 0.5f);
		look_.heightTexture = data.value("heightTexture", "");
		look_.heightScale = data.value("heightScale", 0.05f);
		look_.emissive = ReadVector3(data, "emissive");
		look_.emissiveIntensity = data.value("emissiveIntensity", 1.0f);
		look_.emissiveLightRange = data.value("emissiveLightRange", 0.0f);
		look_.emissiveLightIntensity = data.value("emissiveLightIntensity", 1.0f);
		look_.rim = ReadVector3(data, "rim");
		look_.rimIntensity = data.value("rimIntensity", 1.0f);
		look_.rimPower = data.value("rimPower", 3.0f);
		look_.isMirror = data.value("mirror", false);
		look_.useModelMaterials = data.value("modelMaterials", false);
		look_.light = ReadLight(data, "light");
		flicker_ = EmissiveFlicker::Read(data);
		drive_.reset();
		if (data.contains("drive") && data["drive"].is_object()) {
			const auto& d = data["drive"];
			Drive drive;
			drive.speed = d.value("speed", drive.speed);
			drive.loopMin = d.value("min", drive.loopMin);
			drive.loopMax = d.value("max", drive.loopMax);
			drive_ = drive;
		}
		follow_.reset();
		if (data.contains("follow") && data["follow"].is_object()) {
			const auto& f = data["follow"];
			SplineFollow follow;
			follow.spline = f.value("spline", "");
			follow.speed = f.value("speed", follow.speed);
			follow.offset = f.value("offset", follow.offset);
			follow_ = follow;
		}
		carLights_.reset();
		if (data.contains("carLights")) {
			CarLights lights;
			if (data["carLights"].is_object()) {
				const auto& c = data["carLights"];
				lights.front = ReadVector3(c, "front").value_or(lights.front);
				lights.rear = ReadVector3(c, "rear").value_or(lights.rear);
				lights.headIntensity = c.value("headIntensity", lights.headIntensity);
				lights.headRange = c.value("headRange", lights.headRange);
				lights.tailIntensity = c.value("tailIntensity", lights.tailIntensity);
				lights.tailRange = c.value("tailRange", lights.tailRange);
			}
			if (!data["carLights"].is_boolean() || data["carLights"].get<bool>()) {
				carLights_ = lights;
			}
		}

		meshLooks_.clear();
		if (data.contains("meshes") && data["meshes"].is_array()) {
			for (const auto& m : data["meshes"]) {
				MeshLook look;
				look.index = m.value("index", 0);
				look.color = ReadVector4(m, "color");
				look.roughness = ReadFloat(m, "roughness");
				look.metallic = ReadFloat(m, "metallic");
				look.emissive = ReadVector3(m, "emissive");
				meshLooks_.push_back(look);
			}
		}
		collider_.reset();
		if (data.contains("collider") && data["collider"].is_object()) {
			const auto& c = data["collider"];
			ColliderDesc desc;
			const std::string shape = c.value("shape", std::string("Box"));
			desc.shape = (shape == "Sphere") ? Collision::ColliderShape::Sphere :
				(shape == "Capsule") ? Collision::ColliderShape::Capsule : Collision::ColliderShape::Box;
			desc.size = ReadVector3(c, "size").value_or(desc.size);
			desc.radius = c.value("radius", desc.radius);
			desc.height = c.value("height", desc.height);
			const std::string axis = c.value("axis", std::string("Y"));
			desc.axis = (axis == "X") ? 0 : (axis == "Z") ? 2 : 1;
			desc.offset = ReadVector3(c, "offset").value_or(desc.offset);
			desc.layer = c.value("layer", desc.layer);
			desc.mask = c.value("mask", desc.mask);
			desc.tag = c.value("tag", desc.tag);
			collider_ = desc;
		}

		SetName(data.value("name", look_.model));
		if (data.contains("transform")) {
			const auto& t = data["transform"];
			transform_.translate = ReadVector3(t, "translate").value_or(transform_.translate);
			transform_.rotate = ReadVector3(t, "rotate").value_or(transform_.rotate);
			transform_.scale = ReadVector3(t, "scale").value_or(transform_.scale);
		}
		Build();
		ApplyCollider();
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
		if (!look_.emissiveTexture.empty()) { data["emissiveTexture"] = look_.emissiveTexture; }
		if (look_.color) { data["color"] = { look_.color->x, look_.color->y, look_.color->z, look_.color->w }; }
		if (look_.uvScale) { data["uvScale"] = { look_.uvScale->x, look_.uvScale->y }; }
		if (look_.roughness) { data["roughness"] = *look_.roughness; }
		if (look_.metallic) { data["metallic"] = *look_.metallic; }
		if (look_.environment) { data["environment"] = *look_.environment; }
		if (look_.weathering) { data["weathering"] = *look_.weathering; }
		if (look_.flowWater) { data["flowWater"] = *look_.flowWater; }
		if (look_.clearCoat) {
			data["clearCoat"] = *look_.clearCoat;
			data["clearCoatRoughness"] = look_.clearCoatRoughness;
		}
		if (look_.anisotropy) {
			data["anisotropy"] = *look_.anisotropy;
			data["anisotropyRotation"] = look_.anisotropyRotation;
		}
		if (!look_.heightTexture.empty()) {
			data["heightTexture"] = look_.heightTexture;
			data["heightScale"] = look_.heightScale;
		}
		if (look_.sheen) {
			data["sheen"] = { look_.sheen->x, look_.sheen->y, look_.sheen->z };
			data["sheenRoughness"] = look_.sheenRoughness;
		}
		if (look_.emissive) {
			data["emissive"] = { look_.emissive->x, look_.emissive->y, look_.emissive->z };
			data["emissiveIntensity"] = look_.emissiveIntensity;
			if (look_.emissiveLightRange > 0.0f) {
				data["emissiveLightRange"] = look_.emissiveLightRange;
				data["emissiveLightIntensity"] = look_.emissiveLightIntensity;
			}
		}
		if (look_.rim) {
			data["rim"] = { look_.rim->x, look_.rim->y, look_.rim->z };
			data["rimIntensity"] = look_.rimIntensity;
			data["rimPower"] = look_.rimPower;
		}
		if (look_.isMirror) { data["mirror"] = true; }
		if (look_.useModelMaterials) { data["modelMaterials"] = true; }
		if (look_.light) { data["light"] = LightToName(*look_.light); }
		flicker_.Write(data);
		if (drive_) {
			data["drive"] = { { "speed", drive_->speed }, { "min", drive_->loopMin }, { "max", drive_->loopMax } };
		}
		if (follow_) {
			data["follow"] = { { "spline", follow_->spline }, { "speed", follow_->speed }, { "offset", follow_->offset } };
		}
		if (!meshLooks_.empty()) {
			nlohmann::json meshes = nlohmann::json::array();
			for (const MeshLook& m : meshLooks_) {
				nlohmann::json mesh = { { "index", m.index } };
				if (m.color) { mesh["color"] = { m.color->x, m.color->y, m.color->z, m.color->w }; }
				if (m.roughness) { mesh["roughness"] = *m.roughness; }
				if (m.metallic) { mesh["metallic"] = *m.metallic; }
				if (m.emissive) { mesh["emissive"] = { m.emissive->x, m.emissive->y, m.emissive->z }; }
				meshes.push_back(mesh);
			}
			data["meshes"] = meshes;
		}
		if (collider_) {
			const ColliderDesc& c = *collider_;
			constexpr const char* kShapeNames[] = { "Box", "Sphere", "Capsule" };
			constexpr const char* kAxisNames[] = { "X", "Y", "Z" };
			nlohmann::json collider = { { "shape", kShapeNames[static_cast<int>(c.shape)] } };
			if (c.shape == Collision::ColliderShape::Box) {
				collider["size"] = { c.size.x, c.size.y, c.size.z };
			} else {
				collider["radius"] = c.radius;
			}
			if (c.shape == Collision::ColliderShape::Capsule) {
				collider["height"] = c.height;
				collider["axis"] = kAxisNames[std::clamp(c.axis, 0, 2)];
			}
			if (c.offset.x != 0.0f || c.offset.y != 0.0f || c.offset.z != 0.0f) {
				collider["offset"] = { c.offset.x, c.offset.y, c.offset.z };
			}
			if (c.layer != 1u) { collider["layer"] = c.layer; }
			if (c.mask != Collision::kAllLayers) { collider["mask"] = c.mask; }
			if (!c.tag.empty()) { collider["tag"] = c.tag; }
			data["collider"] = collider;
		}
		if (carLights_) {
			const CarLights& c = *carLights_;
			data["carLights"] = {
				{ "front", { c.front.x, c.front.y, c.front.z } }, { "rear", { c.rear.x, c.rear.y, c.rear.z } },
				{ "headIntensity", c.headIntensity }, { "headRange", c.headRange },
				{ "tailIntensity", c.tailIntensity }, { "tailRange", c.tailRange },
			};
		}
		return data;
	}

	void PlacedObject::Update() {
		time_ += Core::FPSKeeper::DeltaTime();
		// 曲線に沿う設定があればそちらが位置を決める
		if (drive_ && !follow_) {
			UpdateDrive();
		}
		if (animeModel_ && look_.isAnimation) {
			animeModel_->AnimationUpdate();
		}
		// 拡縮を変えたら判定もついてくる
		ApplyColliderSize();
		GameObject::Update();
		if (flicker_.IsEnabled()) {
			ApplyFlicker();
		}
		ApplyEmissiveLight(IsActive());
		ApplyCarLights(IsActive());
		if (drive_ && IsActive()) {
			EmitWheelSpray();
		}
	}

	void PlacedObject::TurnOffLight() {
		ApplyEmissiveLight(false);
		ApplyCarLights(false);
		if (lightManager_) {
			for (int& slot : carLightSlots_) {
				if (slot >= 0) {
					lightManager_->RemoveClusteredLight(slot);
					slot = -1;
				}
			}
		}
	}

	void PlacedObject::SetLookTexture(const std::string& texture) {
		look_.texture = texture;
		ApplyLook();
	}

	void PlacedObject::FollowSpline(const Math::Spline& spline) {
		if (!follow_ || !IsActive() || spline.GetLength() <= 0.0f) {
			return;
		}
		const float length = spline.GetLength();
		follow_->travelled = std::fmod(follow_->travelled + follow_->speed * Core::FPSKeeper::DeltaTime(), length);
		float distance = std::fmod(follow_->offset + follow_->travelled, length);
		if (distance < 0.0f) {
			distance += length;
		}
		Vector3 direction = spline.GetDirectionAtDistance(distance);
		if (follow_->speed < 0.0f) {
			direction = direction * -1.0f;
		}
		transform_.translate = spline.GetLocationAtDistance(distance);
		// 進む向きへ +Z を向ける。X は正で前が下がるので、上り坂は負
		transform_.rotate.y = std::atan2(direction.x, direction.z);
		transform_.rotate.x = -std::asin(std::clamp(direction.y, -1.0f, 1.0f));
	}

	void PlacedObject::UpdateDrive() {
		Vector3 forward = transform_.GetForward();
		forward.y = 0.0f;
		transform_.translate = transform_.translate + forward * (drive_->speed * Core::FPSKeeper::DeltaTime());
		// 進む向きのいちばん大きい軸で、範囲の端から反対の端へ
		float& axis = (std::abs(forward.x) >= std::abs(forward.z)) ? transform_.translate.x : transform_.translate.z;
		const float length = drive_->loopMax - drive_->loopMin;
		if (length > 0.0f) {
			if (axis > drive_->loopMax) {
				axis -= length;
			} else if (axis < drive_->loopMin) {
				axis += length;
			}
		}
	}

	void PlacedObject::EmitWheelSpray() {
		Graphics::RainRenderer* rain = Graphics::ObjectRenderer::GetInstance()->GetRainRenderer();
		if (!rain || !rain->IsSprayActive()) {
			return;
		}
		Vector3 heading = transform_.GetForward();
		heading.y = 0.0f;
		if (heading.Length() < 1e-4f) {
			return;
		}
		const Vector3 velocity = heading.Normalize() * drive_->speed;
		const Vector3 position = transform_.GetWorldPos();
		const Vector3 right = transform_.GetRight() * transform_.scale.x;
		const Vector3 forward = transform_.GetForward() * transform_.scale.z;
		// タイヤの位置 (モデルの座標)
		constexpr float kWheelX = 0.6f;
		constexpr float kWheelZ = 0.85f;
		constexpr float kWheelWidth = 0.28f;
		for (int i = 0; i < 4; i++) {
			const float side = (i % 2 == 0) ? 1.0f : -1.0f;
			const float front = (i < 2) ? 1.0f : -1.0f;
			rain->AddSprayWheel(position + right * (kWheelX * side) + forward * (kWheelZ * front), kWheelWidth * transform_.scale.x, velocity);
		}
	}

	void PlacedObject::ApplyFlicker() {
		RenderObject* object = GetRenderObject();
		if (!object || !look_.emissive) {
			return;
		}
		const Vector3 emissive = *look_.emissive * (look_.emissiveIntensity * flicker_.Evaluate(time_));
		for (size_t i = 0; i < object->GetMaterialCount(); ++i) {
			object->GetMaterial(i).SetEmissive(emissive);
		}
	}

	void PlacedObject::ApplyCarLights(bool isOn) {
		if (!lightManager_ || !carLights_) {
			return;
		}
		if (carLightSlots_[0] < 0) {
			if (!isOn) {
				return;
			}
			for (int& slot : carLightSlots_) {
				slot = lightManager_->AddClusteredLight();
			}
		}
		const CarLights& c = *carLights_;
		const Vector3 position = transform_.GetWorldPos();
		const Vector3 right = transform_.GetRight() * transform_.scale.x;
		const Vector3 up = transform_.GetUp() * transform_.scale.y;
		const Vector3 forward = transform_.GetForward() * transform_.scale.z;
		auto toWorld = [&](const Vector3& local, float side) {
			return position + right * (local.x * side) + up * local.y + forward * local.z;
		};
		// ヘッドライトは少し下へ向ける
		Vector3 beam = transform_.GetForward() - transform_.GetUp() * 0.12f;
		beam = beam.Normalize();
		for (int i = 0; i < 4; i++) {
			Graphics::ClusteredLightData* light = lightManager_->GetClusteredLight(carLightSlots_[i]);
			if (!light) {
				continue;
			}
			const float side = (i % 2 == 0) ? 1.0f : -1.0f;
			if (i < 2) {
				light->type = Graphics::kClusteredLightSpot;
				light->position = toWorld(c.front, side);
				light->direction = beam;
				light->color = Vector3{ 1.0f,0.92f,0.8f } * (isOn ? c.headIntensity : 0.0f);
				light->range = c.headRange;
				light->decay = 1.5f;
				light->cosAngle = std::cos(28.0f * 3.14159265f / 180.0f);
				light->cosFalloffStart = std::cos(16.0f * 3.14159265f / 180.0f);
			} else {
				light->type = Graphics::kClusteredLightPoint;
				light->position = toWorld(c.rear, side);
				light->color = Vector3{ 1.0f,0.05f,0.03f } * (isOn ? c.tailIntensity : 0.0f);
				light->range = c.tailRange;
				light->decay = 2.0f;
			}
		}
	}

	void PlacedObject::ApplyEmissiveLight(bool isOn) {
		if (!lightManager_) {
			return;
		}
		const bool isLit = isOn && look_.emissive && look_.emissiveLightRange > 0.0f;
		if (lightSlot_ < 0) {
			if (!isLit) {
				return;
			}
			Graphics::PointLightData initial;
			initial.intensity = 0.0f;
			lightSlot_ = lightManager_->AddPointLight(initial);
			if (lightSlot_ < 0) {
				return;
			}
		}

		Graphics::PointLightData* light = lightManager_->GetPointLight(lightSlot_);
		if (!isLit) {
			light->intensity = 0.0f;
			return;
		}
		light->color = { look_.emissive->x, look_.emissive->y, look_.emissive->z, 1.0f };
		light->position = transform_.GetWorldPos();
		light->intensity = look_.emissiveIntensity * look_.emissiveLightIntensity * (flicker_.IsEnabled() ? flicker_.Evaluate(time_) : 1.0f);
		light->radius = look_.emissiveLightRange;
		light->decay = 2.0f;
		// 光源は物の中心にあるので、そのままだとこの物自身が光を全部遮る。物の外側から先だけ影を見る
		const float scale = (std::max)({ std::abs(transform_.scale.x), std::abs(transform_.scale.y), std::abs(transform_.scale.z) });
		light->sourceRadius = modelRadius_ * scale * 1.05f;
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
		modelRadius_ = 0.0f;
		if (const RenderObject* object = GetRenderObject(); object && object->GetModel()) {
			for (const Graphics::Mesh& mesh : object->GetModel()->GetMeshes()) {
				modelRadius_ = (std::max)(modelRadius_, mesh.GetBoundingRadius());
			}
		}
		ApplyLook();
		// 動く物は焼く GI と反射プローブに入れない
		if (RenderObject* object = GetRenderObject()) {
			object->SetStaticLighting(!look_.isAnimation && !drive_ && !follow_);
		}
	}

	void PlacedObject::ApplyLook() {
		RenderObject* object = GetRenderObject();
		if (!object) {
			return;
		}
		// 先にモデルの値を当て、データに書いてある項目で上書きする
		if (look_.useModelMaterials) {
			object->ApplyModelMaterials();
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
			material.SetWeathering(look_.weathering.value_or(0.0f));
			material.SetFlowSpeed(look_.flowWater.value_or(0.0f));
			// データに無ければモデルのマテリアルの値 (使わないときは無し)
			if (look_.clearCoat) {
				material.SetClearCoat(*look_.clearCoat, look_.clearCoatRoughness);
			} else if (!look_.useModelMaterials) {
				material.SetClearCoat(0.0f, 0.05f);
			}
			if (look_.anisotropy) {
				material.SetAnisotropy(*look_.anisotropy, look_.anisotropyRotation * (std::numbers::pi_v<float> / 180.0f));
			} else if (!look_.useModelMaterials) {
				material.SetAnisotropy(0.0f, 0.0f);
			}
			material.SetHeightMap(look_.heightTexture, look_.heightScale);
			if (look_.sheen) {
				material.SetSheen(*look_.sheen, look_.sheenRoughness);
			} else if (!look_.useModelMaterials) {
				material.SetSheen(Vector3{}, 0.5f);
			}
			if (!look_.emissiveTexture.empty()) {
				material.SetEmissiveMap(look_.emissiveTexture);
			} else if (!look_.useModelMaterials) {
				material.SetEmissiveMap("");
			}
			// データに無ければ、モデルのマテリアルを使うときはモデルの発光、使わないときは発光無し
			if (look_.emissive) {
				material.SetEmissive(*look_.emissive * look_.emissiveIntensity);
			} else if (!look_.useModelMaterials) {
				material.SetEmissive(Vector3{});
			}
			material.SetRim(look_.rim ? *look_.rim * look_.rimIntensity : Vector3{}, look_.rimPower);
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
		ApplyMeshLooks();
	}

	void PlacedObject::ApplyMeshLooks() {
		RenderObject* object = GetRenderObject();
		if (!object) {
			return;
		}
		for (const MeshLook& look : meshLooks_) {
			if (look.index < 0 || static_cast<size_t>(look.index) >= object->GetMaterialCount()) {
				continue;
			}
			Material& material = object->GetMaterial(static_cast<size_t>(look.index));
			if (look.color) { material.SetColor(*look.color); }
			if (look.roughness) { material.SetRoughness(*look.roughness); }
			if (look.metallic) { material.SetMetallic(*look.metallic); }
			if (look.emissive) { material.SetEmissive(*look.emissive); }
		}
	}

	void PlacedObject::MeshInspectorGUI([[maybe_unused]] int meshIndex) {
	#ifdef _DEBUGMODE
		RenderObject* object = GetRenderObject();
		if (!object || meshIndex < 0 || static_cast<size_t>(meshIndex) >= object->GetMaterialCount()) {
			return;
		}
		Material& material = object->GetMaterial(static_cast<size_t>(meshIndex));
		auto found = std::find_if(meshLooks_.begin(), meshLooks_.end(), [meshIndex](const MeshLook& m) { return m.index == meshIndex; });
		// 変えた項目だけを残す
		auto edit = [&]() -> MeshLook& {
			if (found == meshLooks_.end()) {
				meshLooks_.push_back(MeshLook{ .index = meshIndex });
				found = std::prev(meshLooks_.end());
			}
			return *found;
		};
		Vector4 color = material.GetColor();
		if (ImGui::ColorEdit4("Color##mesh", &color.x)) {
			edit().color = color;
		}
		float roughness = material.GetRoughness();
		if (ImGui::SliderFloat("Roughness##mesh", &roughness, 0.0f, 1.0f)) {
			edit().roughness = roughness;
		}
		float metallic = material.GetMetallic();
		if (ImGui::SliderFloat("Metallic##mesh", &metallic, 0.0f, 1.0f)) {
			edit().metallic = metallic;
		}
		Vector3 emissive = material.GetEmissive();
		if (ImGui::ColorEdit3("Emissive##mesh", &emissive.x, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float)) {
			edit().emissive = emissive;
		}
		ImGui::BeginDisabled(found == meshLooks_.end());
		if (ImGui::Button("Reset##mesh")) {
			meshLooks_.erase(found);
			// 他のメッシュと同じ見た目から当て直す
			Build();
			ImGui::EndDisabled();
			return;
		}
		ImGui::EndDisabled();
		ApplyMeshLooks();
	#endif // _DEBUGMODE
	}

	void PlacedObject::ParameterGUI() {
	#ifdef _DEBUGMODE
		char name[64] = {};
		GetName().copy(name, sizeof(name) - 1);
		if (ImGui::InputText("Name", name, sizeof(name))) {
			SetName(name);
		}
		ImGui::LabelText("Model", "%s", look_.model.c_str());
		// 外したときはモデルを作り直す
		if (ImGui::Checkbox("Model materials", &look_.useModelMaterials)) {
			Build();
			return;
		}

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
		char emissiveTexture[128] = {};
		look_.emissiveTexture.copy(emissiveTexture, sizeof(emissiveTexture) - 1);
		if (ImGui::InputText("Emissive texture", emissiveTexture, sizeof(emissiveTexture), ImGuiInputTextFlags_EnterReturnsTrue)) {
			look_.emissiveTexture = emissiveTexture;
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
		changed |= OptionalGUI("Weathering", look_.weathering, material.GetWeathering(),
			[](const char* label, float& v) { return ImGui::SliderFloat(label, &v, 0.0f, 1.0f); });
		changed |= OptionalGUI("Flow water", look_.flowWater, 0.6f,
			[](const char* label, float& v) { return ImGui::DragFloat(label, &v, 0.01f, -5.0f, 5.0f, "%.2f m/s"); });
		changed |= OptionalGUI("Clear coat", look_.clearCoat, 1.0f,
			[](const char* label, float& v) { return ImGui::SliderFloat(label, &v, 0.0f, 1.0f); });
		if (look_.clearCoat) {
			changed |= ImGui::SliderFloat("Clear coat roughness", &look_.clearCoatRoughness, 0.0f, 1.0f);
		}
		changed |= OptionalGUI("Anisotropy", look_.anisotropy, 0.6f,
			[](const char* label, float& v) { return ImGui::SliderFloat(label, &v, -1.0f, 1.0f); });
		if (look_.anisotropy) {
			changed |= ImGui::SliderFloat("Anisotropy rotation", &look_.anisotropyRotation, -180.0f, 180.0f, "%.0f deg");
		}
		char heightTexture[128] = {};
		look_.heightTexture.copy(heightTexture, sizeof(heightTexture) - 1);
		if (ImGui::InputText("Height texture", heightTexture, sizeof(heightTexture), ImGuiInputTextFlags_EnterReturnsTrue)) {
			look_.heightTexture = heightTexture;
			changed = true;
		}
		if (!look_.heightTexture.empty()) {
			changed |= ImGui::DragFloat("Height scale", &look_.heightScale, 0.002f, 0.0f, 0.5f, "%.3f");
		}
		changed |= OptionalGUI("Sheen", look_.sheen, Vector3{ 1.0f,1.0f,1.0f },
			[](const char* label, Vector3& v) { return ImGui::ColorEdit3(label, &v.x, ImGuiColorEditFlags_Float); });
		if (look_.sheen) {
			changed |= ImGui::SliderFloat("Sheen roughness", &look_.sheenRoughness, 0.0f, 1.0f);
		}
		changed |= OptionalGUI("Emissive", look_.emissive, Vector3{ 1.0f,1.0f,1.0f },
			[](const char* label, Vector3& v) { return ImGui::ColorEdit3(label, &v.x); });
		if (look_.emissive) {
			changed |= ImGui::DragFloat("Emissive intensity", &look_.emissiveIntensity, 0.05f, 0.0f, 100.0f);
			// 0 なら周りを照らさない。照らすと点光源の枠を1つ使う
			ImGui::DragFloat("Emissive light range", &look_.emissiveLightRange, 0.1f, 0.0f, 1000.0f);
			if (look_.emissiveLightRange > 0.0f) {
				ImGui::DragFloat("Emissive light intensity", &look_.emissiveLightIntensity, 0.05f, 0.0f, 100.0f);
				if (lightSlot_ < 0 && lightManager_ && lightManager_->GetNumPointLights() >= Graphics::kMaxPointLights) {
					ImGui::TextDisabled("(ライトの数が上限)");
				}
			}
		}
		changed |= OptionalGUI("Rim", look_.rim, Vector3{ 1.0f,1.0f,1.0f },
			[](const char* label, Vector3& v) { return ImGui::ColorEdit3(label, &v.x); });
		if (look_.rim) {
			changed |= ImGui::DragFloat("Rim intensity", &look_.rimIntensity, 0.05f, 0.0f, 100.0f);
			changed |= ImGui::DragFloat("Rim power", &look_.rimPower, 0.05f, 0.5f, 16.0f);
		}
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
		if (look_.emissive && flicker_.GUI() && !flicker_.IsEnabled()) {
			changed = true;
		}
		if (drive_) {
			ImGui::DragFloat("Drive speed", &drive_->speed, 0.1f, -50.0f, 50.0f, "%.1f m/s");
		}
		// 曲線に沿って進む。名前は Hierarchy の曲線 (Spline) の名前
		bool isFollowing = follow_.has_value();
		if (ImGui::Checkbox("Follow spline", &isFollowing)) {
			if (isFollowing) {
				follow_ = SplineFollow{};
			} else {
				follow_.reset();
			}
			// 動く物は静的な光 (焼く GI・反射プローブ) から外す
			changed = true;
		}
		if (follow_) {
			char splineName[64] = {};
			follow_->spline.copy(splineName, sizeof(splineName) - 1);
			if (ImGui::InputText("Spline name", splineName, sizeof(splineName))) {
				follow_->spline = splineName;
			}
			ImGui::DragFloat("Follow speed", &follow_->speed, 0.1f, -100.0f, 100.0f, "%.1f m/s");
			ImGui::DragFloat("Follow offset", &follow_->offset, 0.1f, -10000.0f, 10000.0f, "%.1f m");
		}

		// あたり判定
		bool hasCollider = collider_.has_value();
		if (ImGui::Checkbox("Collider", &hasCollider)) {
			if (hasCollider) {
				collider_ = ColliderDesc{};
				FitColliderToModel();
			} else {
				collider_.reset();
			}
			ApplyCollider();
		}
		if (collider_) {
			ColliderDesc& c = *collider_;
			bool isColliderChanged = false;
			constexpr const char* kShapeNames[] = { "Box", "Sphere", "Capsule" };
			int shape = static_cast<int>(c.shape);
			if (ImGui::Combo("Collider shape", &shape, kShapeNames, IM_ARRAYSIZE(kShapeNames))) {
				c.shape = static_cast<Collision::ColliderShape>(shape);
				FitColliderToModel();
				isColliderChanged = true;
			}
			if (c.shape == Collision::ColliderShape::Box) {
				ImGui::DragFloat3("Collider size", &c.size.x, 0.01f, 0.0f, 1000.0f);
			} else {
				ImGui::DragFloat("Collider radius", &c.radius, 0.01f, 0.0f, 1000.0f);
			}
			if (c.shape == Collision::ColliderShape::Capsule) {
				ImGui::DragFloat("Collider height", &c.height, 0.01f, 0.0f, 1000.0f);
				constexpr const char* kAxisNames[] = { "X", "Y", "Z" };
				ImGui::Combo("Collider axis", &c.axis, kAxisNames, IM_ARRAYSIZE(kAxisNames));
			}
			ImGui::DragFloat3("Collider offset", &c.offset.x, 0.01f);
			if (ImGui::Button("Fit to model")) {
				FitColliderToModel();
			}
			char tag[64] = {};
			c.tag.copy(tag, sizeof(tag) - 1);
			if (ImGui::InputText("Collider tag", tag, sizeof(tag))) {
				c.tag = tag;
				isColliderChanged = true;
			}
			if (isColliderChanged) {
				ApplyCollider();
			}
			// レイヤーとマスクはコライダーの GUI で変えて、ここへ書き戻す
			if (!colliders_.empty()) {
				colliders_.front()->LayerGUI();
				c.layer = colliders_.front()->GetCollisionLayer();
				c.mask = colliders_.front()->GetCollisionMask();
			}
		}

		if (changed) {
			ApplyLook();
		}
	#endif // _DEBUGMODE
	}

	void PlacedObject::ApplyCollider() {
		colliders_.clear();
		if (!collider_) {
			return;
		}
		Collision::BaseCollider* collider = nullptr;
		switch (collider_->shape) {
		case Collision::ColliderShape::Box: collider = AddCollider(collider_->tag); break;
		case Collision::ColliderShape::Sphere: collider = AddSphereCollider(collider_->tag); break;
		case Collision::ColliderShape::Capsule: collider = AddCapsuleCollider(collider_->tag); break;
		}
		collider->SetParent(&transform_);
		collider->SetCollisionLayer(collider_->layer);
		collider->SetCollisionMask(collider_->mask);
		ApplyColliderSize();
	}

	void PlacedObject::ApplyColliderSize() {
		if (!collider_ || colliders_.empty()) {
			return;
		}
		const ColliderDesc& c = *collider_;
		const Vector3 scale = { std::abs(transform_.scale.x), std::abs(transform_.scale.y), std::abs(transform_.scale.z) };
		const float scales[3] = { scale.x, scale.y, scale.z };
		Collision::BaseCollider* collider = colliders_.front().get();
		// ずれも物の拡縮に合わせる
		collider->SetOffset({ c.offset.x * scale.x, c.offset.y * scale.y, c.offset.z * scale.z });
		switch (c.shape) {
		case Collision::ColliderShape::Box: {
			auto* box = static_cast<Collision::AABBCollider*>(collider);
			box->SetWidth(c.size.x * scale.x);
			box->SetHeight(c.size.y * scale.y);
			box->SetDepth(c.size.z * scale.z);
			break;
		}
		case Collision::ColliderShape::Sphere:
			static_cast<Collision::SphereCollider*>(collider)->SetRadius(c.radius * (std::max)({ scale.x, scale.y, scale.z }));
			break;
		case Collision::ColliderShape::Capsule: {
			auto* capsule = static_cast<Collision::CapsuleCollider*>(collider);
			const int axis = std::clamp(c.axis, 0, 2);
			// 半径は軸と直交する2方向の大きい方
			capsule->SetRadius(c.radius * (std::max)(scales[(axis + 1) % 3], scales[(axis + 2) % 3]));
			capsule->SetHeight(c.height * scales[axis]);
			capsule->SetAxis(static_cast<Collision::CapsuleCollider::Axis>(axis));
			break;
		}
		}
	}

	void PlacedObject::FitColliderToModel() {
		if (!collider_) {
			return;
		}
		const RenderObject* object = GetRenderObject();
		if (!object || !object->GetModel()) {
			return;
		}
		bool hasBounds = false;
		Vector3 min{};
		Vector3 max{};
		for (const Graphics::Mesh& mesh : object->GetModel()->GetMeshes()) {
			Vector3 meshMin;
			Vector3 meshMax;
			if (!mesh.GetBounds(meshMin, meshMax)) {
				continue;
			}
			min = hasBounds ? Vector3{ (std::min)(min.x, meshMin.x), (std::min)(min.y, meshMin.y), (std::min)(min.z, meshMin.z) } : meshMin;
			max = hasBounds ? Vector3{ (std::max)(max.x, meshMax.x), (std::max)(max.y, meshMax.y), (std::max)(max.z, meshMax.z) } : meshMax;
			hasBounds = true;
		}
		if (!hasBounds) {
			return;
		}
		ColliderDesc& c = *collider_;
		const Vector3 extent = max - min;
		const float extents[3] = { extent.x, extent.y, extent.z };
		c.offset = (min + max) * 0.5f;
		c.size = extent;
		if (c.shape == Collision::ColliderShape::Sphere) {
			c.radius = (std::max)({ extent.x, extent.y, extent.z }) * 0.5f;
		} else if (c.shape == Collision::ColliderShape::Capsule) {
			// いちばん長い向きに伸ばす
			c.axis = (extent.x >= extent.y && extent.x >= extent.z) ? 0 : (extent.y >= extent.z ? 1 : 2);
			c.height = extents[c.axis];
			c.radius = (std::max)(extents[(c.axis + 1) % 3], extents[(c.axis + 2) % 3]) * 0.5f;
		}
		ApplyColliderSize();
	}

}
