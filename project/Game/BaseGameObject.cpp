#include "BaseGameObject.h"

BaseGameObject::BaseGameObject() {
}

void BaseGameObject::Initialize() {
	model_ = std::make_unique<Object3d>();
}

void BaseGameObject::Update() {
}

void BaseGameObject::Draw(Material* mate) {
	model_->Draw(mate);
}

#ifdef _DEBUG
void BaseGameObject::Debug() {
}
#endif // _DEBUG

void BaseGameObject::SetModel(const std::string& name) {
	model_->Create(name);
}
