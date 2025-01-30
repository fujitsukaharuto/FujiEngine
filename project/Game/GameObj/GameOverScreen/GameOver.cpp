#include"GameOver.h"


void GameOver::Init() {
	overPaneru_ = std::make_unique<Sprite>();
	overPaneru_->Load("gameover.png");
	overPaneru_->SetAnchor({ 0.0f,0.0f });
}

void GameOver::Update() {

}

void GameOver::Draw() {
	overPaneru_->Draw();
}