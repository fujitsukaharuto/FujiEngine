#include "Level.h"
#include <algorithm>
#include <numbers>
#include <filesystem>
#include <json.hpp>
#include "Engine/GameObject/PlacedObject.h"
#include "Engine/GameObject/SpawnPoint.h"
#include "Engine/GameObject/PlacedLight.h"
#include "Engine/Graphics/Sprite/PlacedSprite.h"
#include "Engine/Core/Serialize/JsonSerializer.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/Graphics/SkyBox/SkyBox.h"
#include "Engine/Graphics/Light/LightManager.h"
#include "Engine/Graphics/Model/ModelManager.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/Graphics/PostEffect/OffscreenManager.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Camera/DebugCamera.h"

using namespace Core;
using namespace Math;
using namespace Scene;


namespace {
	const std::filesystem::path kLevelDirectory = "resource/Json/Level";

	std::string LevelPath(const std::string& name) {
		return (kLevelDirectory / (name + ".json")).string();
	}

	Vector3 ReadVector3(const nlohmann::json& data, const char* key, const Vector3& fallback) {
		if (!data.contains(key) || !data[key].is_array() || data[key].size() < 3) {
			return fallback;
		}
		const auto& v = data[key];
		return { v[0].get<float>(), v[1].get<float>(), v[2].get<float>() };
	}

	Vector4 ReadVector4(const nlohmann::json& data, const char* key, const Vector4& fallback) {
		if (!data.contains(key) || !data[key].is_array() || data[key].size() < 4) {
			return fallback;
		}
		const auto& v = data[key];
		return { v[0].get<float>(), v[1].get<float>(), v[2].get<float>(), v[3].get<float>() };
	}

	// 配置データには名前で書く。並びは Graphics::TonemapMode と同じ
	constexpr const char* kTonemapNames[] = { "None","Reinhard","ACES","AgX" };
	constexpr int kTonemapCount = static_cast<int>(std::size(kTonemapNames));

	// 並びは Graphics::RayTracedReflectionMode と同じ
	constexpr const char* kReflectionNames[] = { "Off","Occlusion","Objects" };
	constexpr int kReflectionCount = static_cast<int>(std::size(kReflectionNames));

	// 並びは Graphics::RayTracedAOMode / RayTracedShadowMode と同じ
	constexpr const char* kAONames[] = { "Off","Inline","Screen" };
	constexpr int kAOCount = static_cast<int>(std::size(kAONames));
	constexpr const char* kShadowNames[] = { "Hard","Soft" };
	constexpr int kShadowCount = static_cast<int>(std::size(kShadowNames));
	// 並びは Graphics::AmbientMode と同じ
	constexpr const char* kAmbientNames[] = { "Hemisphere","IBL" };
	constexpr int kAmbientCount = static_cast<int>(std::size(kAmbientNames));
	// 並びは Graphics::SkyMode と同じ
	constexpr const char* kSkyNames[] = { "Texture","Atmosphere" };
	constexpr int kSkyCount = static_cast<int>(std::size(kSkyNames));
	// 並びは Graphics::LightingMode と同じ
	constexpr const char* kLightingNames[] = { "Simple","PBR" };
	constexpr int kLightingCount = static_cast<int>(std::size(kLightingNames));

	/// <summary>名前から番号を引く。見つからなければ fallback</summary>
	int IndexOfName(const std::string& name, const char* const* names, int count, int fallback) {
		for (int i = 0; i < count; ++i) {
			if (name == names[i]) {
				return i;
			}
		}
		return fallback;
	}

	Graphics::OffscreenManager* GetOffscreen() {
		DXC::DXCom* dxcommon = Graphics::ModelManager::GetInstance()->ShareDXCom();
		return dxcommon ? dxcommon->GetOffscreenManager() : nullptr;
	}

	Scene::Environment ReadEnvironment(const nlohmann::json& data) {
		Scene::Environment env;
		env.hasSkyBox = data.value("skyBox", env.hasSkyBox);
		env.skyBoxColor = ReadVector4(data, "skyBoxColor", env.skyBoxColor);
		env.lightColor = ReadVector4(data, "lightColor", env.lightColor);
		env.lightDirection = ReadVector3(data, "lightDirection", env.lightDirection);
		env.lightIntensity = data.value("lightIntensity", env.lightIntensity);
		env.cameraPosition = ReadVector3(data, "cameraPosition", env.cameraPosition);
		env.cameraRotate = ReadVector3(data, "cameraRotate", env.cameraRotate);
		env.fogColor = ReadVector3(data, "fogColor", env.fogColor);
		env.fogColorFromSky = data.value("fogColorFromSky", env.fogColorFromSky);
		env.fogSkyColorScale = data.value("fogSkyColorScale", env.fogSkyColorScale);
		env.fogDensity = data.value("fogDensity", env.fogDensity);
		env.fogHeightFalloff = data.value("fogHeightFalloff", env.fogHeightFalloff);
		env.fogBaseHeight = data.value("fogBaseHeight", env.fogBaseHeight);
		env.fogStartDistance = data.value("fogStartDistance", env.fogStartDistance);
		env.fogMaxOpacity = data.value("fogMaxOpacity", env.fogMaxOpacity);
		env.fogSkyAmount = data.value("fogSkyAmount", env.fogSkyAmount);
		env.fogSunScatter = data.value("fogSunScatter", env.fogSunScatter);
		env.fogSunPower = data.value("fogSunPower", env.fogSunPower);
		env.bloomEnabled = data.value("bloom", env.bloomEnabled);
		env.bloomThreshold = data.value("bloomThreshold", env.bloomThreshold);
		env.bloomIntensity = data.value("bloomIntensity", env.bloomIntensity);
		env.bloomKnee = data.value("bloomKnee", env.bloomKnee);
		env.bloomScatter = data.value("bloomScatter", env.bloomScatter);
		const std::string tonemapName = data.value("tonemap", std::string(kTonemapNames[env.tonemapMode]));
		for (int i = 0; i < kTonemapCount; ++i) {
			if (tonemapName == kTonemapNames[i]) {
				env.tonemapMode = i;
			}
		}
		env.exposure = data.value("exposure", env.exposure);
		env.saturation = data.value("saturation", env.saturation);
		env.contrast = data.value("contrast", env.contrast);
		env.sharpen = data.value("sharpen", env.sharpen);
		env.vignette = data.value("vignette", env.vignette);
		env.shadowTint = ReadVector3(data, "shadowTint", env.shadowTint);
		env.highlightTint = ReadVector3(data, "highlightTint", env.highlightTint);
		env.godRayIntensity = data.value("godRayIntensity", env.godRayIntensity);
		env.godRayLength = data.value("godRayLength", env.godRayLength);
		env.godRaySunSize = data.value("godRaySunSize", env.godRaySunSize);
		env.aoMode = IndexOfName(data.value("ao", std::string(kAONames[env.aoMode])), kAONames, kAOCount, env.aoMode);
		env.shadowMode = IndexOfName(data.value("shadow", std::string(kShadowNames[env.shadowMode])), kShadowNames, kShadowCount, env.shadowMode);
		env.ambientMode = IndexOfName(data.value("ambient", std::string(kAmbientNames[env.ambientMode])), kAmbientNames, kAmbientCount, env.ambientMode);
		env.iblAmbientScale = data.value("iblAmbientScale", env.iblAmbientScale);
		env.skyMode = IndexOfName(data.value("sky", std::string(kSkyNames[env.skyMode])), kSkyNames, kSkyCount, env.skyMode);
		env.skyIntensity = data.value("skyIntensity", env.skyIntensity);
		env.lightingMode = IndexOfName(data.value("lighting", std::string(kLightingNames[env.lightingMode])), kLightingNames, kLightingCount, env.lightingMode);
		const std::string reflectionName = data.value("reflection", std::string(kReflectionNames[env.reflectionMode]));
		for (int i = 0; i < kReflectionCount; ++i) {
			if (reflectionName == kReflectionNames[i]) {
				env.reflectionMode = i;
			}
		}
		env.reflectionMaxDistance = data.value("reflectionMaxDistance", env.reflectionMaxDistance);
		env.reflectionIntensity = data.value("reflectionIntensity", env.reflectionIntensity);
		env.reflectionShadow = data.value("reflectionShadow", env.reflectionShadow);
		env.dofBlur = data.value("dofBlur", env.dofBlur);
		env.dofFocusDistance = data.value("dofFocusDistance", env.dofFocusDistance);
		env.dofFocusRange = data.value("dofFocusRange", env.dofFocusRange);
		env.dofFalloff = data.value("dofFalloff", env.dofFalloff);
		return env;
	}

	nlohmann::json ToJson(const Scene::Environment& env) {
		nlohmann::json data;
		data["skyBox"] = env.hasSkyBox;
		data["skyBoxColor"] = { env.skyBoxColor.x, env.skyBoxColor.y, env.skyBoxColor.z, env.skyBoxColor.w };
		data["lightColor"] = { env.lightColor.x, env.lightColor.y, env.lightColor.z, env.lightColor.w };
		data["lightDirection"] = { env.lightDirection.x, env.lightDirection.y, env.lightDirection.z };
		data["lightIntensity"] = env.lightIntensity;
		data["cameraPosition"] = { env.cameraPosition.x, env.cameraPosition.y, env.cameraPosition.z };
		data["cameraRotate"] = { env.cameraRotate.x, env.cameraRotate.y, env.cameraRotate.z };
		data["fogColor"] = { env.fogColor.x, env.fogColor.y, env.fogColor.z };
		data["fogColorFromSky"] = env.fogColorFromSky;
		data["fogSkyColorScale"] = env.fogSkyColorScale;
		data["fogDensity"] = env.fogDensity;
		data["fogHeightFalloff"] = env.fogHeightFalloff;
		data["fogBaseHeight"] = env.fogBaseHeight;
		data["fogStartDistance"] = env.fogStartDistance;
		data["fogMaxOpacity"] = env.fogMaxOpacity;
		data["fogSkyAmount"] = env.fogSkyAmount;
		data["fogSunScatter"] = env.fogSunScatter;
		data["fogSunPower"] = env.fogSunPower;
		data["bloom"] = env.bloomEnabled;
		data["bloomThreshold"] = env.bloomThreshold;
		data["bloomIntensity"] = env.bloomIntensity;
		data["bloomKnee"] = env.bloomKnee;
		data["bloomScatter"] = env.bloomScatter;
		data["tonemap"] = kTonemapNames[std::clamp(env.tonemapMode, 0, kTonemapCount - 1)];
		data["exposure"] = env.exposure;
		data["saturation"] = env.saturation;
		data["contrast"] = env.contrast;
		data["sharpen"] = env.sharpen;
		data["vignette"] = env.vignette;
		data["shadowTint"] = { env.shadowTint.x, env.shadowTint.y, env.shadowTint.z };
		data["highlightTint"] = { env.highlightTint.x, env.highlightTint.y, env.highlightTint.z };
		data["godRayIntensity"] = env.godRayIntensity;
		data["godRayLength"] = env.godRayLength;
		data["godRaySunSize"] = env.godRaySunSize;
		data["ao"] = kAONames[std::clamp(env.aoMode, 0, kAOCount - 1)];
		data["shadow"] = kShadowNames[std::clamp(env.shadowMode, 0, kShadowCount - 1)];
		data["ambient"] = kAmbientNames[std::clamp(env.ambientMode, 0, kAmbientCount - 1)];
		data["iblAmbientScale"] = env.iblAmbientScale;
		data["sky"] = kSkyNames[std::clamp(env.skyMode, 0, kSkyCount - 1)];
		data["skyIntensity"] = env.skyIntensity;
		data["lighting"] = kLightingNames[std::clamp(env.lightingMode, 0, kLightingCount - 1)];
		data["reflection"] = kReflectionNames[std::clamp(env.reflectionMode, 0, kReflectionCount - 1)];
		data["reflectionMaxDistance"] = env.reflectionMaxDistance;
		data["reflectionIntensity"] = env.reflectionIntensity;
		data["reflectionShadow"] = env.reflectionShadow;
		data["dofBlur"] = env.dofBlur;
		data["dofFocusDistance"] = env.dofFocusDistance;
		data["dofFocusRange"] = env.dofFocusRange;
		data["dofFalloff"] = env.dofFalloff;
		return data;
	}
}


Level::Level() = default;

Level::~Level() = default;

void Level::Load(const std::string& name, Graphics::LightManager* lightManager) {
	name_ = name;
	lightManager_ = lightManager;
	objects_.clear();
	spawnPoints_.clear();
	lights_.clear();
	sprites_.clear();
	environment_ = {};

	const nlohmann::json data = JsonSerializer::DeserializeJsonData(LevelPath(name));
	if (data.is_object() && data.contains("environment")) {
		environment_ = ReadEnvironment(data["environment"]);
	}
	// ファイルが無くても既定の環境は入れる。前のシーンの空やライトを残さないため
	if (lightManager_) {
		lightManager_->ResetLights();
	}
	// 前のシーンが途中で足した効果(ラジアルブラーやビネット)も持ち越さない
	if (Graphics::OffscreenManager* offscreen = GetOffscreen()) {
		offscreen->ResetPostEffect();
	}
	ApplyEnvironment();
	ApplyCamera();

	if (!data.is_object()) {
		return;
	}
	if (data.contains("objects")) {
		for (const auto& objectData : data["objects"]) {
			auto object = std::make_unique<GameObject::PlacedObject>();
			object->SetLightManager(lightManager_);
			object->Create(objectData);
			objects_.push_back(std::move(object));
		}
	}
	if (data.contains("spawnPoints")) {
		for (const auto& pointData : data["spawnPoints"]) {
			auto point = std::make_unique<GameObject::SpawnPoint>();
			point->Create(pointData);
			spawnPoints_.push_back(std::move(point));
		}
	}
	// ResetLights の後なので、ゲームが使う点光源0番の次から並ぶ
	if (data.contains("lights")) {
		for (const auto& lightData : data["lights"]) {
			auto light = std::make_unique<GameObject::PlacedLight>();
			light->Create(lightData, lightManager_);
			lights_.push_back(std::move(light));
		}
	}
	if (data.contains("sprites")) {
		for (const auto& spriteData : data["sprites"]) {
			auto sprite = std::make_unique<Graphics::PlacedSprite>();
			sprite->Create(spriteData);
			sprites_.push_back(std::move(sprite));
		}
	}
}

void Level::Save() const {
	nlohmann::json objects = nlohmann::json::array();
	for (const auto& object : objects_) {
		objects.push_back(object->ToJson());
	}
	nlohmann::json spawnPoints = nlohmann::json::array();
	for (const auto& point : spawnPoints_) {
		spawnPoints.push_back(point->ToJson());
	}
	nlohmann::json lights = nlohmann::json::array();
	for (const auto& light : lights_) {
		lights.push_back(light->ToJson());
	}
	nlohmann::json sprites = nlohmann::json::array();
	for (const auto& sprite : sprites_) {
		sprites.push_back(sprite->ToJson());
	}
	nlohmann::json data;
	data["environment"] = ToJson(environment_);
	data["objects"] = std::move(objects);
	data["spawnPoints"] = std::move(spawnPoints);
	data["lights"] = std::move(lights);
	data["sprites"] = std::move(sprites);

	std::filesystem::create_directories(kLevelDirectory);
	JsonSerializer::SerializeJsonData(data, LevelPath(name_));
}

void Level::Update() {
	for (auto& object : objects_) {
		object->Update();
	}
	for (auto& light : lights_) {
		light->Update();
	}
}

void Level::Draw() {
	if (environment_.hasSkyBox && skyBox_) {
		skyBox_->Draw();
	}
	for (auto& object : objects_) {
		object->Draw();
	}
	for (auto& point : spawnPoints_) {
		point->Draw();
	}
	for (auto& light : lights_) {
		light->Draw();
	}
}

void Level::DrawSprites() {
	for (auto& sprite : sprites_) {
		sprite->Draw();
	}
}

GameObject::PlacedObject* Level::Add(const std::string& modelName) {
	auto object = std::make_unique<GameObject::PlacedObject>();
	object->SetLightManager(lightManager_);
	object->Create(modelName);
	objects_.push_back(std::move(object));
	return objects_.back().get();
}

GameObject::GameObject* Level::Duplicate(const GameObject::GameObject* source) {
	// 書き出したデータから作り直す。保存される項目だけが写るので、保存→読み込みと同じ結果になる
	for (const auto& object : objects_) {
		if (object.get() == source) {
			auto copy = std::make_unique<GameObject::PlacedObject>();
			copy->SetLightManager(lightManager_);
			copy->Create(object->ToJson());
			objects_.push_back(std::move(copy));
			return objects_.back().get();
		}
	}
	for (const auto& point : spawnPoints_) {
		if (point.get() == source) {
			auto copy = std::make_unique<GameObject::SpawnPoint>();
			copy->Create(point->ToJson());
			spawnPoints_.push_back(std::move(copy));
			return spawnPoints_.back().get();
		}
	}
	for (const auto& light : lights_) {
		if (light.get() == source) {
			auto copy = std::make_unique<GameObject::PlacedLight>();
			copy->Create(light->ToJson(), lightManager_);
			lights_.push_back(std::move(copy));
			return lights_.back().get();
		}
	}
	return nullptr;
}

bool Level::Remove(const GameObject::GameObject* object) {
	const auto isTarget = [object](const auto& o) { return o.get() == object; };
	for (auto& light : lights_) {
		if (light.get() == object) {
			light->TurnOff();
		}
	}
	for (auto& placed : objects_) {
		if (placed.get() == object) {
			placed->TurnOffLight();
		}
	}
	return std::erase_if(objects_, isTarget) > 0 || std::erase_if(spawnPoints_, isTarget) > 0 || std::erase_if(lights_, isTarget) > 0;
}

bool Level::Contains(const GameObject::GameObject* object) const {
	const auto isTarget = [object](const auto& o) { return o.get() == object; };
	return std::any_of(objects_.begin(), objects_.end(), isTarget) || std::any_of(spawnPoints_.begin(), spawnPoints_.end(), isTarget) ||
		std::any_of(lights_.begin(), lights_.end(), isTarget);
}

GameObject::PlacedLight* Level::AddLight(bool isSpot) {
	auto light = std::make_unique<GameObject::PlacedLight>();
	light->Create(isSpot ? GameObject::PlacedLight::Type::Spot : GameObject::PlacedLight::Type::Point, lightManager_);
	lights_.push_back(std::move(light));
	return lights_.back().get();
}

GameObject::SpawnPoint* Level::AddSpawnPoint() {
	spawnPoints_.push_back(std::make_unique<GameObject::SpawnPoint>());
	return spawnPoints_.back().get();
}

const GameObject::SpawnPoint* Level::FindSpawnPoint(const std::string& entry) const {
	if (spawnPoints_.empty()) {
		return nullptr;
	}
	for (const auto& point : spawnPoints_) {
		if (!entry.empty() && point->GetName() == entry) {
			return point.get();
		}
	}
	return spawnPoints_.front().get();
}

Graphics::PlacedSprite* Level::AddSprite(const std::string& texture) {
	auto sprite = std::make_unique<Graphics::PlacedSprite>();
	sprite->Create(texture);
	sprites_.push_back(std::move(sprite));
	return sprites_.back().get();
}

bool Level::RemoveSprite(const Graphics::PlacedSprite* sprite) {
	return std::erase_if(sprites_, [sprite](const auto& s) { return s.get() == sprite; }) > 0;
}

bool Level::ContainsSprite(const Graphics::PlacedSprite* sprite) const {
	return std::any_of(sprites_.begin(), sprites_.end(), [sprite](const auto& s) { return s.get() == sprite; });
}

Graphics::PlacedSprite* Level::FindSprite(const std::string& name) {
	for (auto& sprite : sprites_) {
		if (sprite->GetName() == name) {
			return sprite.get();
		}
	}
	return nullptr;
}

GameObject::PlacedObject* Level::Find(const std::string& name) {
	for (auto& object : objects_) {
		if (object->GetName() == name) {
			return object.get();
		}
	}
	return nullptr;
}

void Level::ApplyEnvironment() {
	if (environment_.hasSkyBox) {
		if (!skyBox_) {
			skyBox_ = std::make_unique<Graphics::SkyBox>();
			skyBox_->Initialize();
		}
		skyBox_->SetColor(environment_.skyBoxColor);
	}

	if (lightManager_) {
		Graphics::DirectionalLight* light = lightManager_->GetDirectionLight(0);
		light->SetColor(environment_.lightColor);
		// 長さ0だと正規化できないので真下にする
		if (environment_.lightDirection.Length() > 0.0001f) {
			light->SetLightDirection(environment_.lightDirection);
		} else {
			light->SetLightDirection({ 0.0f,-1.0f,0.0f });
		}
		light->SetLightIntensity(environment_.lightIntensity);

		const bool isAtmosphere = environment_.hasSkyBox && environment_.skyMode == Graphics::kSkyModeAtmosphere;
		const Math::Vector3 sunDirection = light->direction * -1.0f;
		const float sunPower = light->intensity * environment_.skyIntensity;
		const Math::Vector3 sunColor = {
			light->color.x * sunPower * environment_.skyBoxColor.x,
			light->color.y * sunPower * environment_.skyBoxColor.y,
			light->color.z * sunPower * environment_.skyBoxColor.z };
		const Math::Vector3 kHorizonDirections[4] = { { 1.0f,0.0f,0.0f },{ -1.0f,0.0f,0.0f },{ 0.0f,0.0f,1.0f },{ 0.0f,0.0f,-1.0f } };

		Graphics::FogData fog;
		const bool isFromSky = environment_.fogColorFromSky && environment_.hasSkyBox && skyBox_;
		fog.color = {};
		for (int i = 0; i < 4; ++i) {
			Math::Vector3 color = environment_.fogColor;
			if (isFromSky) {
				color = isAtmosphere ? Graphics::SkyBox::AtmosphereColor(kHorizonDirections[i], sunDirection, sunColor) : skyBox_->GetHorizonColor(i);
				color.x *= environment_.fogSkyColorScale;
				color.y *= environment_.fogSkyColorScale;
				color.z *= environment_.fogSkyColorScale;
			}
			fog.horizonColor[i] = { color.x, color.y, color.z, 0.0f };
			fog.color.x += color.x * 0.25f;
			fog.color.y += color.y * 0.25f;
			fog.color.z += color.z * 0.25f;
		}
		fog.density = environment_.fogDensity;
		fog.heightFalloff = environment_.fogHeightFalloff;
		fog.baseHeight = environment_.fogBaseHeight;
		fog.startDistance = environment_.fogStartDistance;
		fog.maxOpacity = environment_.fogMaxOpacity;
		fog.skyAmount = environment_.fogSkyAmount;
		fog.sunScatter = environment_.fogSunScatter;
		fog.sunScatterPower = environment_.fogSunPower;
		fog.skyMode = isAtmosphere ? Graphics::kSkyModeAtmosphere : Graphics::kSkyModeTexture;
		fog.skyIntensity = environment_.skyIntensity;
		lightManager_->SetFog(fog);

		lightManager_->SetRayTracingModes(
			static_cast<Graphics::RayTracedAOMode>(std::clamp(environment_.aoMode, 0, kAOCount - 1)),
			static_cast<Graphics::RayTracedShadowMode>(std::clamp(environment_.shadowMode, 0, kShadowCount - 1)));
		// 計算する空のときは、焼いた IBL が空と合わないので半球の2色を空から作る
		Graphics::AmbientMode ambientMode = static_cast<Graphics::AmbientMode>(std::clamp(environment_.ambientMode, 0, kAmbientCount - 1));
		const Graphics::AllLightsData kDefaultLights{};
		Math::Vector3 ambientSky = kDefaultLights.ambientSkyColor;
		Math::Vector3 ambientGround = kDefaultLights.ambientGroundColor;
		if (isAtmosphere) {
			ambientMode = Graphics::kAmbientModeHemisphere;
			Math::Vector3 horizon{};
			for (const Math::Vector3& direction : kHorizonDirections) {
				horizon = horizon + Graphics::SkyBox::AtmosphereColor(direction, sunDirection, sunColor) * 0.25f;
			}
			const Math::Vector3 zenith = Graphics::SkyBox::AtmosphereColor({ 0.0f,1.0f,0.0f }, sunDirection, sunColor);
			ambientSky = zenith * 0.75f + horizon * 0.25f;
			ambientGround = horizon * 0.3f;
		}
		lightManager_->SetAmbientColors(ambientSky, ambientGround);
		lightManager_->SetAmbientMode(ambientMode, environment_.iblAmbientScale);
		lightManager_->SetLightingMode(
			static_cast<Graphics::LightingMode>(std::clamp(environment_.lightingMode, 0, kLightingCount - 1)));
		lightManager_->SetEnvironmentSky(
			{ environment_.skyBoxColor.x, environment_.skyBoxColor.y, environment_.skyBoxColor.z },
			environment_.hasSkyBox && skyBox_ != nullptr);
		lightManager_->SetReflection(
			static_cast<Graphics::RayTracedReflectionMode>(std::clamp(environment_.reflectionMode, 0, kReflectionCount - 1)),
			environment_.reflectionMaxDistance, environment_.reflectionIntensity, environment_.reflectionShadow);
	}

	if (Graphics::OffscreenManager* offscreen = GetOffscreen()) {
		Graphics::PostProcessSettings post;
		post.bloomEnabled = environment_.bloomEnabled;
		post.bloomThreshold = environment_.bloomThreshold;
		post.bloomIntensity = environment_.bloomIntensity;
		post.bloomKnee = environment_.bloomKnee;
		post.bloomScatter = environment_.bloomScatter;
		post.tonemapMode = static_cast<Graphics::TonemapMode>(std::clamp(environment_.tonemapMode, 0, kTonemapCount - 1));
		post.exposure = environment_.exposure;
		post.saturation = environment_.saturation;
		post.contrast = environment_.contrast;
		post.sharpen = environment_.sharpen;
		post.vignette = environment_.vignette;
		post.shadowTint = environment_.shadowTint;
		post.highlightTint = environment_.highlightTint;
		post.godRayIntensity = environment_.godRayIntensity;
		post.godRayLength = environment_.godRayLength;
		post.godRaySunSize = environment_.godRaySunSize;
		post.dofBlur = environment_.dofBlur;
		post.dofFocusDistance = environment_.dofFocusDistance;
		post.dofFocusRange = environment_.dofFocusRange;
		post.dofFalloff = environment_.dofFalloff;
		offscreen->ApplySettings(post);
	}
}

void Level::ApplyCamera() {
	Math::Trans& t = Graphics::CameraManager::GetInstance()->GetCamera()->GetTransform();
	t.translate = environment_.cameraPosition;
	t.rotate = environment_.cameraRotate;
}

void Level::EnvironmentGUI() {
#ifdef _DEBUGMODE
	bool changed = false;
	ImGui::SeparatorText("Quality");
	// 右へ行くほど豪華で重い。Standard が従来の既定
	struct QualityPreset { const char* name; int shadow; int reflection; int ambient; };
	constexpr QualityPreset kPresets[] = {
		{ "Standard", Graphics::kShadowModeHard, Graphics::kReflectionModeOff, Graphics::kAmbientModeHemisphere },
		{ "High", Graphics::kShadowModeSoft, Graphics::kReflectionModeOcclusion, Graphics::kAmbientModeHemisphere },
		{ "Ultra", Graphics::kShadowModeSoft, Graphics::kReflectionModeObjects, Graphics::kAmbientModeHemisphere },
		{ "Realistic", Graphics::kShadowModeSoft, Graphics::kReflectionModeObjects, Graphics::kAmbientModeIBL },
	};
	for (const QualityPreset& preset : kPresets) {
		if (ImGui::Button(preset.name)) {
			environment_.aoMode = Graphics::kAOModeScreen;
			environment_.shadowMode = preset.shadow;
			environment_.reflectionMode = preset.reflection;
			environment_.ambientMode = preset.ambient;
			changed = true;
		}
		ImGui::SameLine();
	}
	ImGui::NewLine();
	changed |= ImGui::Combo("Lighting", &environment_.lightingMode, kLightingNames, kLightingCount);
	changed |= ImGui::Combo("AO",&environment_.aoMode, kAONames, kAOCount);
	changed |= ImGui::Combo("Shadow", &environment_.shadowMode, kShadowNames, kShadowCount);
	changed |= ImGui::Combo("Ambient", &environment_.ambientMode, kAmbientNames, kAmbientCount);
	ImGui::BeginDisabled(environment_.ambientMode != Graphics::kAmbientModeIBL);
	changed |= ImGui::DragFloat("IBL scale", &environment_.iblAmbientScale, 0.01f, 0.0f, 10.0f);
	ImGui::EndDisabled();

	ImGui::SeparatorText("SkyBox");
	changed |= ImGui::Checkbox("Enable", &environment_.hasSkyBox);
	ImGui::BeginDisabled(!environment_.hasSkyBox);
	changed |= ImGui::Combo("Type##sky", &environment_.skyMode, kSkyNames, kSkyCount);
	changed |= ImGui::DragFloat("Intensity##sky", &environment_.skyIntensity, 0.01f, 0.0f, 10.0f);
	changed |= ImGui::ColorEdit3("Color##sky", &environment_.skyBoxColor.x);
	ImGui::EndDisabled();

	ImGui::SeparatorText("Directional Light");
	changed |= ImGui::ColorEdit3("Color##light", &environment_.lightColor.x);
	changed |= ImGui::DragFloat3("Direction", &environment_.lightDirection.x, 0.01f);
	changed |= ImGui::DragFloat("Intensity", &environment_.lightIntensity, 0.01f, 0.0f, 100.0f);

	ImGui::SeparatorText("Fog");
	changed |= ImGui::Checkbox("Color from sky", &environment_.fogColorFromSky);
	const bool isFogFromSky = environment_.fogColorFromSky && environment_.hasSkyBox;
	ImGui::BeginDisabled(!isFogFromSky);
	changed |= ImGui::DragFloat("Sky color scale", &environment_.fogSkyColorScale, 0.01f, 0.0f, 10.0f);
	ImGui::EndDisabled();
	ImGui::BeginDisabled(isFogFromSky);
	// 空の暗さに合わせると 0.05 前後の値になるので、0〜255 の刻みでは粗すぎる
	changed |= ImGui::ColorEdit3("Color##fog", &environment_.fogColor.x, ImGuiColorEditFlags_Float);
	ImGui::EndDisabled();
	changed |= ImGui::DragFloat("Density", &environment_.fogDensity, 0.0002f, 0.0f, 1.0f, "%.4f");
	changed |= ImGui::DragFloat("Height falloff", &environment_.fogHeightFalloff, 0.001f, 0.0f, 2.0f, "%.3f");
	changed |= ImGui::DragFloat("Base height", &environment_.fogBaseHeight, 0.1f);
	changed |= ImGui::DragFloat("Start distance", &environment_.fogStartDistance, 0.5f, 0.0f, 10000.0f);
	changed |= ImGui::SliderFloat("Max opacity", &environment_.fogMaxOpacity, 0.0f, 1.0f);
	changed |= ImGui::SliderFloat("Sky amount", &environment_.fogSkyAmount, 0.0f, 1.0f);
	changed |= ImGui::DragFloat("Sun scatter", &environment_.fogSunScatter, 0.01f, 0.0f, 10.0f);
	changed |= ImGui::DragFloat("Sun power", &environment_.fogSunPower, 0.1f, 1.0f, 64.0f);

	ImGui::SeparatorText("Bloom");
	changed |= ImGui::Checkbox("Enable##bloom", &environment_.bloomEnabled);
	ImGui::BeginDisabled(!environment_.bloomEnabled);
	changed |= ImGui::DragFloat("Intensity##bloom", &environment_.bloomIntensity, 0.01f, 0.0f, 10.0f);
	changed |= ImGui::DragFloat("Threshold", &environment_.bloomThreshold, 0.01f, 0.0f, 10.0f);
	changed |= ImGui::DragFloat("Knee", &environment_.bloomKnee, 0.005f, 0.0f, 1.0f);
	changed |= ImGui::SliderFloat("Scatter", &environment_.bloomScatter, 0.0f, 0.95f);
	ImGui::EndDisabled();

	ImGui::SeparatorText("Tonemap");
	changed |= ImGui::Combo("Operator", &environment_.tonemapMode, kTonemapNames, kTonemapCount);
	changed |= ImGui::DragFloat("Exposure", &environment_.exposure, 0.01f, 0.0f, 100.0f);
	changed |= ImGui::DragFloat("Saturation", &environment_.saturation, 0.01f, 0.0f, 2.0f);
	changed |= ImGui::DragFloat("Contrast", &environment_.contrast, 0.01f, 0.5f, 2.0f);
	changed |= ImGui::SliderFloat("Sharpen", &environment_.sharpen, 0.0f, 2.0f);
	changed |= ImGui::SliderFloat("Vignette", &environment_.vignette, 0.0f, 1.0f);
	changed |= ImGui::ColorEdit3("Shadow tint", &environment_.shadowTint.x, ImGuiColorEditFlags_Float);
	changed |= ImGui::ColorEdit3("Highlight tint", &environment_.highlightTint.x, ImGuiColorEditFlags_Float);

	ImGui::SeparatorText("God Ray");
	changed |= ImGui::DragFloat("Intensity##godRay", &environment_.godRayIntensity, 0.01f, 0.0f, 10.0f);
	changed |= ImGui::SliderFloat("Length##godRay", &environment_.godRayLength, 0.05f, 1.0f);
	changed |= ImGui::SliderFloat("Sun size##godRay", &environment_.godRaySunSize, 0.05f, 2.0f);

	ImGui::SeparatorText("Reflection");
	changed |= ImGui::Combo("Mode##reflection", &environment_.reflectionMode, kReflectionNames, kReflectionCount);
	changed |= ImGui::DragFloat("Max distance##reflection", &environment_.reflectionMaxDistance, 1.0f, 0.0f, 10000.0f);
	changed |= ImGui::SliderFloat("Intensity##reflection", &environment_.reflectionIntensity, 0.0f, 2.0f);
	changed |= ImGui::SliderFloat("Shadow##reflection", &environment_.reflectionShadow, 0.0f, 1.0f);

	ImGui::SeparatorText("Depth of Field");
	changed |= ImGui::SliderFloat("Blur##dof", &environment_.dofBlur, 0.0f, 16.0f, "%.1f px");
	changed |= ImGui::DragFloat("Focus distance##dof", &environment_.dofFocusDistance, 0.1f, 0.0f, 10000.0f);
	changed |= ImGui::DragFloat("Focus range##dof", &environment_.dofFocusRange, 0.1f, 0.0f, 10000.0f);
	changed |= ImGui::DragFloat("Falloff##dof", &environment_.dofFalloff, 0.1f, 0.01f, 10000.0f);
	if (changed) {
		ApplyEnvironment();
	}

	ImGui::SeparatorText("Camera");
	bool cameraChanged = false;
	cameraChanged |= ImGui::DragFloat3("Position", &environment_.cameraPosition.x, 0.01f);
	constexpr float kRadToDeg = 180.0f / std::numbers::pi_v<float>;
	Vector3 degrees = environment_.cameraRotate * kRadToDeg;
	if (ImGui::DragFloat3("Rotation", &degrees.x, 0.5f)) {
		environment_.cameraRotate = degrees * (1.0f / kRadToDeg);
		cameraChanged = true;
	}
	if (cameraChanged) {
		ApplyCamera();
	}
	// デバッグカメラで見つけた視点をそのまま初期位置にできる
	if (ImGui::Button("今のカメラを取り込む")) {
		Graphics::CameraManager* cameraManager = Graphics::CameraManager::GetInstance();
		if (cameraManager->GetDebugMode()) {
			environment_.cameraPosition = Graphics::DebugCamera::GetInstance()->GetTranslate();
			environment_.cameraRotate = Graphics::DebugCamera::GetInstance()->GetRotate();
		} else {
			const Math::Trans& t = cameraManager->GetCamera()->GetTransform();
			environment_.cameraPosition = t.translate;
			environment_.cameraRotate = t.rotate;
		}
	}
#endif // _DEBUGMODE
}
