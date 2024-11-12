#include "BaseScene.h"

BaseScene::BaseScene() {
}

void BaseScene::Initialize() {
}

void BaseScene::Update() {
}

void BaseScene::Draw() {
}

void BaseScene::SpriteDraw() {
}

void BaseScene::Init() {
	dxCommon_ = DXCom::GetInstance();
	input_ = Input::GetInstance();
	audio_ = Audio::GetInstance();
}
