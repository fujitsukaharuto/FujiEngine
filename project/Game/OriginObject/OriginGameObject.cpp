#include "OriginGameObject.h"
#include "Engine/Editor/CommandManager.h"
#include "Engine/Editor/PropertyCommand.h"
#include "Engine/Editor/JsonSerializer.h"

using namespace Core;
using namespace Graphics;
using namespace Math;


OriginGameObject::OriginGameObject() {
}

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
	ImGui::Indent();
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Selected;
	if (ImGui::TreeNodeEx("Trans", flags)) {
		ImGui::DragFloat3("position", &model_->GetTransform().translate.x, 0.01f);
		CreatePropertyCommand(0);
		ImGui::DragFloat3("rotate", &model_->GetTransform().rotate.x, 0.01f);
		CreatePropertyCommand(1);
		ImGui::DragFloat3("scale", &model_->GetTransform().scale.x, 0.01f);
		CreatePropertyCommand(2);

		ImGui::Separator();
		gizmo_.DrawOperationRadio();
		JsonSerializer::ShowSaveTransformPopup(model_->GetTransform()); ImGui::SameLine();
		JsonSerializer::ShowLoadTransformPopup(model_->GetTransform());
		gizmo_.Manipulate(model_->GetTransform());

		ImGui::TreePop();
	}
	ImGui::Unindent();
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
	std::string modelName = modelDataJson_.value("modelName", "DefaultModel");
	model_->Create(modelName);
	if (modelDataJson_.contains("transform")) {
		const auto& t = modelDataJson_["transform"];
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
	modelDataJson_ = jsonData;
}

void OriginGameObject::CreatePropertyCommand(int type) {
#ifdef _DEBUGMODE
	if (ImGui::IsItemActivated()) {
		switch (type) {
		case 0: prevPos_ = model_->GetTransform().translate; break;
		case 1: prevRotate_ = model_->GetTransform().rotate;    break;
		case 2: prevScale_ = model_->GetTransform().scale;     break;
		default: break;
		}
	}
	if (ImGui::IsItemDeactivatedAfterEdit()) { // 編集完了検出
		switch (type) {
		case 0:
			CommandManager::TryCreatePropertyCommand(model_->GetTransform(), prevPos_, model_->GetTransform().translate, &Trans::translate);
			prevPos_ = model_->GetTransform().translate;
			break;
		case 1:
			CommandManager::TryCreatePropertyCommand(model_->GetTransform(), prevRotate_, model_->GetTransform().rotate, &Trans::rotate);
			prevRotate_ = model_->GetTransform().rotate;
			break;
		case 2:
			CommandManager::TryCreatePropertyCommand(model_->GetTransform(), prevScale_, model_->GetTransform().scale, &Trans::scale);
			prevScale_ = model_->GetTransform().scale;
			break;
		default: break;
		}
	}
#endif // _DEBUG
}
