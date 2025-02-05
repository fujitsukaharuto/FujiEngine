#include "ResultAnimationManager.h"
#include"GameObj/Timer/Timer.h"
#include"Input/Input.h"
#include"FPSKeeper.h"
#include <iostream>

ResultAnimationManager::ResultAnimationManager()
{

}
///--------------------------------------------------------------------------
/// 初期化
///--------------------------------------------------------------------------
void ResultAnimationManager::Init()
{
    for (size_t i = 0; i < backSprite_.size(); i++) {
        backSprite_[i] = std::make_unique<Sprite>();
        backSprite_[i]->Load("GameTexture/Title_Back.png");
        backSprite_[i]->SetAnchor(Vector2(0.0f, 0.0f));
    }

    scrollSpeed_ = 100.0f;
    backSprite_[0]->SetPos(Vector3(-kTeexWidth, 0, 0));
    backSprite_[1]->SetPos(Vector3(0, 0.0f, 0));
    scroolTime_ = 0.0f;

    font_ = std::make_unique<ResultFont>();
    fontA = std::make_unique<ResultFontA>();

    Reset();
}


///--------------------------------------------------------------------------
/// 更新
///--------------------------------------------------------------------------
void ResultAnimationManager::Update()
{
    scroolTime_ += scrollSpeed_*FPSKeeper::NormalDeltaTime();
    if (scroolTime_ >= kTeexWidth) {
        scroolTime_ = 0.0f;
    }
    backSprite_[0]->SetPos(Vector3(-kTeexWidth + scroolTime_, 0, 0));
    backSprite_[1]->SetPos(Vector3(scroolTime_,0 , 0));

    // ステップごとに更新
    (this->*spFuncTable_[static_cast<size_t>(step_)])();
}

///--------------------------------------------------------------------------
/// 描画
///--------------------------------------------------------------------------
void ResultAnimationManager::Draw()
{
    font_->Draw();
    fontA->Draw();
}

void ResultAnimationManager::BackDraw() {
    for (size_t i = 0; i < backSprite_.size(); i++) {
    
        backSprite_[i]->Draw();
    }
}

///-------------------------------------------------------------------------
///  調節
///-------------------------------------------------------------------------
void ResultAnimationManager::AdjustParamater()
{
    font_->AdjustParm();
    fontA->AdjustParm();
    fontA->AdaptState();

    if (Input::GetInstance()->TriggerKey(DIK_R)) {
        Reset();
    }
}

///-------------------------------------------------------------------------
///  ボムアニメーション
///-------------------------------------------------------------------------
void ResultAnimationManager::FirstAnimation()
{
    font_->Update();
    if (!font_->IsAnimationFinished())return;
    step_ = Step::BOTTOM;
}

///-------------------------------------------------------------------------
///  ボタンアニメーション
///-------------------------------------------------------------------------
void ResultAnimationManager::BottomAnimation()
{
    timerT_ += FPSKeeper::NormalDeltaTime();
    timerPosX = EaseOutBack(timerPosSX, timerPosEX, timerT_, timerTM_);
    pTimer_->SetPos(Vector3(timerPosX, 432.0f, 0.0f), Vector3(timerPosX  +230, 500.0f, 0.0f));
    if (timerT_ < timerTM_)return;
  
    step_ = Step::AUI;
}
///-------------------------------------------------------------------------
///  ボタンアニメーション
///-------------------------------------------------------------------------
void ResultAnimationManager::AUIMove()
{
    fontA->Update();
    if (!fontA->IsAnimationFinished())return;
  
    step_ = Step::ALL;
}
///-------------------------------------------------------------------------
/// 全てアニメーション
///-------------------------------------------------------------------------
void ResultAnimationManager::AllAnimation()
{
    fontA->ExPationing();
}

///-------------------------------------------------------------------------
///  ポインタテーブル
///-------------------------------------------------------------------------
void (ResultAnimationManager::* ResultAnimationManager::spFuncTable_[])()
{
    &ResultAnimationManager::FirstAnimation,
    & ResultAnimationManager::BottomAnimation,
        & ResultAnimationManager::AUIMove,
    & ResultAnimationManager::AllAnimation,
};

///-------------------------------------------------------------------------
///  リセット
///-------------------------------------------------------------------------
void ResultAnimationManager::Reset()
{
    font_->Init();
    fontA->Init();

     timerT_ = 0.0f;
     timerTM_=0.7f;
     timerPosX=0.0f;
     timerPosSX = -490.0f;
     timerPosEX = 316.0f;

    step_ = Step::BOM;
}

void ResultAnimationManager::SetTimer(Timer* timer) {
    pTimer_ = timer;
    pTimer_->SetPos(Vector3(timerPosSX, 432.0f, 0.0f), Vector3(timerPosSX+230, 500.0f, 0.0f));
}