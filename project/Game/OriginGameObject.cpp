#include "OriginGameObject.h"

OriginGameObject::OriginGameObject() {
}

void OriginGameObject::Initialize() {
	model_ = std::make_unique<Object3d>();
}

void OriginGameObject::Update() {
}

void OriginGameObject::Draw(Material* mate) {
	model_->Draw(mate);
}

void OriginGameObject::DebugGUI() {
#ifdef _DEBUG
	ImGui::Indent();
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Selected;
	if (ImGui::TreeNodeEx("Trans", flags)) {
		ImGui::DragFloat3("position", &model_->transform.translate.x, 0.01f);
		ImGui::DragFloat3("rotate", &model_->transform.rotate.x, 0.01f);
		ImGui::DragFloat3("scale", &model_->transform.scale.x, 0.01f);
		ImGui::TreePop();
	}
	ImGui::Unindent();
#endif // _DEBUG
}

#ifdef _DEBUG
void OriginGameObject::Debug() {
}
#endif // _DEBUG

float OriginGameObject::ComparNum(float a, float b) {
	return (a < b) ? a : b;
}

void OriginGameObject::CreateModel(const std::string& name) {
	model_->Create(name);
}

void OriginGameObject::SetModel(const std::string& name) {
	model_->SetModel(name);
}
