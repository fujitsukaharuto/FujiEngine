#include "Unit.h"
#include "Game/GameObj/Selector/Selector.h"

Unit::~Unit() {
}

void Unit::Initialize() {
	OriginGameObject::Initialize();
	model_->CreateSphere();
	model_->transform.translate.z = 10.0f;
}

void Unit::Update() {
	OriginGameObject::Update();
}

void Unit::Draw(Material* mate) {
	OriginGameObject::Draw(mate);
}

void Unit::Move(int x, int y) {
	model_->transform.translate = { float(x),float(y),10.0f };
}

void Unit::MoveEnd(Selector* selector) {
	selector->Move(int(model_->transform.translate.x), int(model_->transform.translate.y));
}
