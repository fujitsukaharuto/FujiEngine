#include "OriginGameObject.h"

OriginGameObject::OriginGameObject() {
}

void OriginGameObject::Initialize() {
	model_ = std::make_unique<Object3d>();
	shadow_ = std::make_unique<Object3d>();
	shadow_->Create("shadow.obj");
	shadow_->SetColor({ 0.05f,0.05f,0.05f,1.0f });
}

void OriginGameObject::Update() {
}

void OriginGameObject::Draw(Material* mate) {
	model_->Draw(mate);
}

void OriginGameObject::ShdowDraw(Material* mate) {
	shadow_->transform.translate = { model_->transform.translate.x,0.01f,model_->transform.translate.z };
	shadow_->Draw(mate);
}

#ifdef _DEBUG
void OriginGameObject::Debug() {
}
#endif // _DEBUG

void OriginGameObject::SetModel(const std::string& name) {
	model_->Create(name);
}
