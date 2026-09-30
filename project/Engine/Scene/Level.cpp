#include "Level.h"
#include <algorithm>
#include <numbers>
#include <filesystem>
#include <json.hpp>
#include "Engine/GameObject/PlacedObject.h"
#include "Engine/GameObject/SpawnPoint.h"
#include "Engine/Graphics/Sprite/PlacedSprite.h"
#include "Engine/Core/Serialize/JsonSerializer.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/Graphics/SkyBox/SkyBox.h"
#include "Engine/Graphics/Light/LightManager.h"
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

	Scene::Environment ReadEnvironment(const nlohmann::json& data) {
		Scene::Environment env;
		env.hasSkyBox = data.value("skyBox", env.hasSkyBox);
		env.skyBoxColor = ReadVector4(data, "skyBoxColor", env.skyBoxColor);
		env.lightColor = ReadVector4(data, "lightColor", env.lightColor);
		env.lightDirection = ReadVector3(data, "lightDirection", env.lightDirection);
		env.lightIntensity = data.value("lightIntensity", env.lightIntensity);
		env.cameraPosition = ReadVector3(data, "cameraPosition", env.cameraPosition);
		env.cameraRotate = ReadVector3(data, "cameraRotate", env.cameraRotate);
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
	ApplyEnvironment();
	ApplyCamera();

	if (!data.is_object()) {
		return;
	}
	if (data.contains("objects")) {
		for (const auto& objectData : data["objects"]) {
			auto object = std::make_unique<GameObject::PlacedObject>();
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
	nlohmann::json sprites = nlohmann::json::array();
	for (const auto& sprite : sprites_) {
		sprites.push_back(sprite->ToJson());
	}
	nlohmann::json data;
	data["environment"] = ToJson(environment_);
	data["objects"] = std::move(objects);
	data["spawnPoints"] = std::move(spawnPoints);
	data["sprites"] = std::move(sprites);

	std::filesystem::create_directories(kLevelDirectory);
	JsonSerializer::SerializeJsonData(data, LevelPath(name_));
}

void Level::Update() {
	for (auto& object : objects_) {
		object->Update();
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
}

void Level::DrawSprites() {
	for (auto& sprite : sprites_) {
		sprite->Draw();
	}
}

GameObject::PlacedObject* Level::Add(const std::string& modelName) {
	auto object = std::make_unique<GameObject::PlacedObject>();
	object->Create(modelName);
	objects_.push_back(std::move(object));
	return objects_.back().get();
}

bool Level::Remove(const GameObject::GameObject* object) {
	const auto isTarget = [object](const auto& o) { return o.get() == object; };
	return std::erase_if(objects_, isTarget) > 0 || std::erase_if(spawnPoints_, isTarget) > 0;
}

bool Level::Contains(const GameObject::GameObject* object) const {
	const auto isTarget = [object](const auto& o) { return o.get() == object; };
	return std::any_of(objects_.begin(), objects_.end(), isTarget) || std::any_of(spawnPoints_.begin(), spawnPoints_.end(), isTarget);
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
	ImGui::SeparatorText("SkyBox");
	changed |= ImGui::Checkbox("Enable", &environment_.hasSkyBox);
	ImGui::BeginDisabled(!environment_.hasSkyBox);
	changed |= ImGui::ColorEdit3("Color##sky", &environment_.skyBoxColor.x);
	ImGui::EndDisabled();

	ImGui::SeparatorText("Directional Light");
	changed |= ImGui::ColorEdit3("Color##light", &environment_.lightColor.x);
	changed |= ImGui::DragFloat3("Direction", &environment_.lightDirection.x, 0.01f);
	changed |= ImGui::DragFloat("Intensity", &environment_.lightIntensity, 0.01f, 0.0f, 100.0f);
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
