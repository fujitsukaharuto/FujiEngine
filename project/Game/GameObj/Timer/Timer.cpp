#include"Timer.h"
#include"DX/FPSKeeper.h"

// 初期化
void Timer::Init() {
	for (int i = 0; i < sprites_.size(); i++) {
		sprites_[i] = std::make_unique<Sprite>();
		sprites_[i]->Load("GameTexture/TimeNumber.png");
		sprites_[i]->SetColor({ 1.0f,1.0f,1.0f,1.0f });
		sprites_[i]->SetAnchor({ 0.0f,0.0f });
		sprites_[i]->SetSize({ kTextureWidth,kTextureHeight }); // サイズを固定
		textureIndex_[i] = 0;
	}
	time_ = 0;
	timeCount_ = 0;
}

// 更新
void Timer::Update() {
	time_ += FPSKeeper::DeltaTimeRate();
}

void Timer::SetTextureRangeForDigit() {
	// 分と秒を取得
	int timeValue = static_cast<int>(time_);
	int minute = timeValue / 60; // 分を算出
	int second = timeValue % 60; // 秒を算出

	// 各桁の数字を計算
	int num3 = second % 10;          /// 1桁（秒）
	int num2 = second / 10 % 10;     /// 2桁（秒）
	int num1 = minute % 100 % 10;  	/// 3桁（分）

	// 表示する数字を確定
	textureIndex_[static_cast<int>(Digit::FIRST)] = num1;
	textureIndex_[static_cast<int>(Digit::SECOND)] = num2;
	textureIndex_[static_cast<int>(Digit::THIRD)] = num3;

	// 描画範囲をセット
	for (int i = 0; i < sprites_.size(); i++) {
		sprites_[i]->SetRange({ kTextureWidth * textureIndex_[i] ,0 }, { kTextureWidth,kTextureHeight });
	}
}

// 描画
void Timer::Draw() {
	for (int i = 0; i < sprites_.size(); i++) {
		sprites_[i]->Draw();
	}
}

void Timer::SetPos(const Vector3& pos) {
	float offset = kTextureWidth * 1.2f; //数字間のオフセット
	for (int i = 0; i < sprites_.size(); i++) {
		sprites_[i]->SetPos({ pos.x + offset * i,pos.y,pos.z });
	}
}