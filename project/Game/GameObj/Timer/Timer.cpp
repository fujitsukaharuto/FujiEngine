#include"Timer.h"
#include"DX/FPSKeeper.h"

// 初期化
void Timer::Init() {
	for (int i = 0; i < sprites_.size(); i++) {
		sprites_[i] = std::make_unique<Sprite>();
		sprites_[i]->Load("GameTexture/TimeNumber.png");
		sprites_[i]->SetColor({ 0.0f,0.0f,0.0f,1.0f });
		sprites_[i]->SetAnchor({ 0.0f,0.0f });
	}
}

// 更新
void Timer::Update() {
	time_ += FPSKeeper::DeltaTimeRate();

}

// 描画
void Timer::Draw() {

}