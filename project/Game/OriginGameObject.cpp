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
}

#ifdef _DEBUG
void OriginGameObject::Debug() {
}
#endif // _DEBUG

void OriginGameObject::CreateModel(const std::string& name) {
	model_->Create(name);
}

void OriginGameObject::SetModel(const std::string& name) {
	model_->SetModel(name);
}
