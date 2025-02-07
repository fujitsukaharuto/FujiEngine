#include "Selector.h"
#include "Game/GameObj/Unit/Unit.h"

Selector::Selector() {
}

Selector::~Selector() {
	delete unit_;
}

void Selector::Initialize() {
	OriginGameObject::Initialize();
	model_->CreateSphere();
	model_->transform.translate.z = 10.0f;
	model_->transform.scale.z = 1.2f;
	model_->SetColor({ 1.0f,0.0f,0.0f,1.0f });

	unit_ = new Unit();
	unit_->Initialize();
}

void Selector::Update() {
	OriginGameObject::Update();
	unit_->Update();
}

void Selector::Draw(Material* mate) {
	if (selectMode_ != SelectMode::UNIT) {
		OriginGameObject::Draw(mate);
	}
	unit_->Draw();
}

void Selector::Move(int x, int y) {
	model_->transform.translate = { float(x),float(y),10.0f };
}

void Selector::SelectUnit() {
	if (model_->transform.translate == unit_->GetTrans().translate) {
		selectMode_ = SelectMode::UNIT;
		selectedUnit_ = unit_;
	}
}

void Selector::UnSelectUnit() {
	selectMode_ = SelectMode::SELECTOR;
	selectedUnit_ = nullptr;
}
