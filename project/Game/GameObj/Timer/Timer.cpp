#include"Timer.h"
#include"DX/FPSKeeper.h"

#include<imgui.h>

float Timer::time_ = 0.0f;
// 初期化
void Timer::Init() {

	// タイムの点々
	timeDotSprite_ = std::make_unique<Sprite>();
	timeDotSprite_->Load("GameTexture/TimeDot.png");

	// 数字
	for (int i = 0; i < sprites_.size(); i++) {
		sprites_[i] = std::make_unique<Sprite>();
		sprites_[i]->Load("GameTexture/TimeNumber.png");
		sprites_[i]->SetColor({ 1.0f,1.0f,1.0f,1.0f });
		sprites_[i]->SetAnchor({ 0.0f,0.0f });
		sprites_[i]->SetSize({ kTextureWidth,kTextureHeight }); // サイズを固定
		textureIndex_[i] = 0;
	}

	
	basePos_ = { 863.0f,162.0f,0.0f };
	dotPos_ = { 951.0f,196.7f,0.0f };
	offset_ = 49.5f; //数字間のオフセット
}

// 更新
void Timer::Update() {
	// 秒加算
	Timer::time_ += FPSKeeper::DeltaTimeRate();
}

void Timer::SetTextureRangeForDigit() {
	
	for (int i = 0; i < sprites_.size()-2; i++) {
		sprites_[i]->SetPos({ basePos_.x + offset_ * i-20,basePos_.y,basePos_.z });
	}
	for (int i = int(sprites_.size()) - 2; i < int(sprites_.size()); i++) {
		sprites_[i]->SetPos({ basePos_.x + offset_ * i,basePos_.y,basePos_.z });
	}

	//dot
	timeDotSprite_->SetPos(dotPos_);

	// 分と秒を取得
	int timeValue = static_cast<int>(Timer::time_);
	int minute = timeValue / 60; // 分
	int second = timeValue % 60; // 秒を算出

	// 各桁の数字を計算
	int s1 = second % 10;              /// 1桁（秒）
	int s2 = second / 10 % 10;         /// 2桁（秒）
	int m1 = minute % 10;  	           /// 1桁（分）
	int m2 = minute/10 % 10;  	       /// 2桁（分）

	// 表示する数字を確定
	textureIndex_[static_cast<int>(Digit::MFIRST)]  = m1;
	textureIndex_[static_cast<int>(Digit::MSECOND)] = m2;
	textureIndex_[static_cast<int>(Digit::SFIRST)]  = s1;
	textureIndex_[static_cast<int>(Digit::SSECOND)] = s2;


	// 描画範囲をセット
	for (int i = 0; i < sprites_.size(); i++) {
		sprites_[i]->SetRange({ kTextureWidth * textureIndex_[i] ,0 }, { kTextureWidth,kTextureHeight });
	}
}

// 描画
void Timer::Draw() {
	/// time
	for (int i = 0; i < sprites_.size(); i++) {
		sprites_[i]->Draw();
	}

	/// dot
	timeDotSprite_->Draw();
}

void Timer::SetPos(const Vector3& pos, const Vector3& dotPos) {
	basePos_ = pos;
	dotPos_ = dotPos;
}

void Timer::SetScale(const float& scale, const float& dots) {

	for (int i = 0; i < sprites_.size(); i++) {
		sprites_[i]->SetSize({ kTextureWidth* scale,kTextureHeight* scale });
	}

	timeDotSprite_->SetSize({ kTextureHeight * dots,kTextureHeight * dots });
}

void Timer ::SetOffSet(const float& offset) {
	offset_ = offset;
}

void Timer::Debug(){
	if(ImGui::CollapsingHeader("Time")) {
		ImGui::PushID("Time");
		ImGui::DragFloat3("BasePos", &basePos_.x, 0.1f);
		ImGui::DragFloat3("DotPos",  &dotPos_.x,  0.1f);
		ImGui::DragFloat("Offset", &offset_, 0.1f);

		ImGui::PopID();
			
	}
}

void Timer::SetTimerZero() {
	Timer::time_ = 0.0f;
}