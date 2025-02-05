#include"BackGround.h"
#include"Easing.h"
#include<imgui.h>

///----------------------------------------------
/// 初期化
///----------------------------------------------
void BackGround::Init() {
	sprite_ = std::make_unique<Sprite>();
	sprite_->Load("GameTexture/Back.png");
	sprite_->SetAnchor({ 0.0f,0.0f });

	/// leftTop
	startScrollPos_ = 930.0f;
	endScrollPos_ = 0.0f;
	leftTopPos_ = startScrollPos_;
}

///----------------------------------------------
/// 更新
///----------------------------------------------
void BackGround::Update() {
	sprite_->SetRange(Vector2(0.0f, leftTopPos_), Vector2(textureWidth, textureHeigth));
}

///----------------------------------------------
/// 描画
///----------------------------------------------
void BackGround::Draw() {
	sprite_->Draw();
}

///----------------------------------------------
/// スクロール
///----------------------------------------------
void BackGround::Scrool(const float& time, const float& maxTime) {

	leftTopPos_ = EaseInBack(startScrollPos_, endScrollPos_, time, maxTime);

	if (time < maxTime) return;
	leftTopPos_ = endScrollPos_;
}

void BackGround::Debug() {
	if(ImGui::CollapsingHeader("BackGround")){

		ImGui::DragFloat("LeftTop", &leftTopPos_, 0.5f);
	}
}