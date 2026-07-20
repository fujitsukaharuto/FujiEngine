#include "OriginGameObject.h"
#include <json.hpp>
#include "Engine/Serialize/JsonSerializer.h"

using namespace Core;
using namespace Graphics;
using namespace Math;
using namespace Editor;


OriginGameObject::OriginGameObject() : modelDataJson_(std::make_unique<nlohmann::json>()) {
}

OriginGameObject::~OriginGameObject() = default;

void OriginGameObject::Initialize() {
	model_ = std::make_unique<Object3d>();
	animeModel_ = std::make_unique<AnimationModel>();
}

void OriginGameObject::Update() {
}

void OriginGameObject::Draw(bool is) {
	if (model_) {
		model_->Draw(is);
	}
}

void OriginGameObject::DebugGUI() {
#ifdef _DEBUGMODE
	// 編集UIはObject3d側(Object3dEditor)に集約されている
	if (model_) {
		model_->DebugGUI();
	}
#endif // _DEBUG
}

#ifdef _DEBUGMODE
void OriginGameObject::Debug() {
}
#endif // _DEBUG

float OriginGameObject::ComparNum(float a, float b) {
	return (a < b) ? a : b;
}

void OriginGameObject::CreateModel(const std::string& name) {
	model_->Create(name);
}

void OriginGameObject::CreateAnimeModel(const std::string& name) {
	animeModel_->Create(name);
}

void OriginGameObject::CreateFromJson(const std::string& name) {
	nlohmann::json objJson = JsonSerializer::DeserializeJsonData(name);
	std::string modelName = objJson.value("modelName", "DefaultModel");
	model_->Create(modelName);
	if (objJson.contains("transform")) {
		const auto& t = objJson["transform"];
		if (t.contains("translate")) {
			model_->GetTransform().translate.x = t["translate"][0];
			model_->GetTransform().translate.y = t["translate"][1];
			model_->GetTransform().translate.z = t["translate"][2];
		}
		if (t.contains("rotate")) {
			model_->GetTransform().rotate.x = t["rotate"][0];
			model_->GetTransform().rotate.y = t["rotate"][1];
			model_->GetTransform().rotate.z = t["rotate"][2];
		}
		if (t.contains("scale")) {
			model_->GetTransform().scale.x = t["scale"][0];
			model_->GetTransform().scale.y = t["scale"][1];
			model_->GetTransform().scale.z = t["scale"][2];
		}
	}
}

void OriginGameObject::CreateFromJson() {
	std::string modelName = modelDataJson_->value("modelName", "DefaultModel");
	model_->Create(modelName);
	if (modelDataJson_->contains("transform")) {
		const auto& t = (*modelDataJson_)["transform"];
		if (t.contains("translate")) {
			model_->GetTransform().translate.x = t["translate"][0];
			model_->GetTransform().translate.y = t["translate"][1];
			model_->GetTransform().translate.z = t["translate"][2];
		}
		if (t.contains("rotate")) {
			model_->GetTransform().rotate.x = t["rotate"][0];
			model_->GetTransform().rotate.y = t["rotate"][1];
			model_->GetTransform().rotate.z = t["rotate"][2];
		}
		if (t.contains("scale")) {
			model_->GetTransform().scale.x = t["scale"][0];
			model_->GetTransform().scale.y = t["scale"][1];
			model_->GetTransform().scale.z = t["scale"][2];
		}
	}
}

void OriginGameObject::SetModel(const std::string& name) {
	model_->SetModel(name);
}

void OriginGameObject::SetAnimeModel(const std::string& name) {
	animeModel_->SetModel(name);
}

void OriginGameObject::SetModelDataJson(const nlohmann::json& jsonData) {
	*modelDataJson_ = jsonData;
}

