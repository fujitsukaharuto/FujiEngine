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
	// 描画オブジェクトはここで両方作らない。
	// CreateModel系が呼ばれたときに必要な方だけ生成する
	// (両方作ると、片方しか使わないオブジェクトが空のObject3d/AnimationModelを抱え続け、
	//  GetTransが「一度もCreateされていない方」のTransformを静かに返す事故も起きる)
}

Graphics::Object3d* OriginGameObject::EnsureModel() {
	if (!model_) {
		model_ = std::make_unique<Object3d>();
	}
	return model_.get();
}

Graphics::AnimationModel* OriginGameObject::EnsureAnimeModel() {
	if (!animeModel_) {
		animeModel_ = std::make_unique<AnimationModel>();
	}
	return animeModel_.get();
}

void OriginGameObject::Update() {
}

void OriginGameObject::Draw(bool is) {
	if (model_) {
		model_->Draw(is);
	}
	for (auto& r : renderers_) {
		r->Draw(is);
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
	EnsureModel()->Create(name);
}

void OriginGameObject::CreateAnimeModel(const std::string& name) {
	EnsureAnimeModel()->Create(name);
}

void OriginGameObject::CreateFromJson(const std::string& name) {
	nlohmann::json objJson = JsonSerializer::DeserializeJsonData(name);
	std::string modelName = objJson.value("modelName", "DefaultModel");
	EnsureModel()->Create(modelName);
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
	EnsureModel()->Create(modelName);
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
	EnsureModel()->SetModel(name);
}

void OriginGameObject::SetAnimeModel(const std::string& name) {
	EnsureAnimeModel()->SetModel(name);
}

void OriginGameObject::SetModelDataJson(const nlohmann::json& jsonData) {
	*modelDataJson_ = jsonData;
}

Graphics::Object3d* OriginGameObject::AddRenderer() {
	auto obj = std::make_unique<Object3d>();
	Object3d* handle = obj.get();
	renderers_.push_back(std::move(obj));
	return handle;
}

Graphics::Object3d* OriginGameObject::AddRenderer(const std::string& name) {
	Object3d* handle = AddRenderer();
	handle->Create(name);
	return handle;
}

