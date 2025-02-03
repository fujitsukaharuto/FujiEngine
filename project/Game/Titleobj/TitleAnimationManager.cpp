#include "TitleAnimationManager.h"
#include "TitleBom.h"
#include "TItleBottom.h"
#include"Input/Input.h"
#include"FPSKeeper.h"
#include <iostream>

TitleAnimationManager::TitleAnimationManager()
{

}
///--------------------------------------------------------------------------
/// 初期化
///--------------------------------------------------------------------------
void TitleAnimationManager::Init() 
{
    for (size_t i = 0; i < backSprite_.size(); i++) {
        backSprite_[i] = std::make_unique<Sprite>();
        backSprite_[i]->Load("GameTexture/Title_Back.png");
        backSprite_[i]->SetAnchor(Vector2(0.0f, 0.0f));
     
    }
    scrollSpeed_ = 100.0f;
    backSprite_[0]->SetPos(Vector3(0, -kTeexHeigth, 0));
    backSprite_[1]->SetPos(Vector3(0, 0.0f, 0));
    scroolTime_ = 0.0f;

    titleBom_ = std::make_unique<TitleBom>();
    titleBottom_ = std::make_unique<TitleBottom>();

    Reset();
}


///--------------------------------------------------------------------------
/// 更新
///--------------------------------------------------------------------------
void TitleAnimationManager::Update()
{
    scroolTime_ += scrollSpeed_*FPSKeeper::NormalDeltaTime();
    if (scroolTime_ >= kTeexHeigth) {
        scroolTime_ = 0.0f;
    }
    backSprite_[0]->SetPos(Vector3(0, -kTeexHeigth+ scroolTime_, 0));
    backSprite_[1]->SetPos(Vector3(0, scroolTime_, 0));

    // ステップごとに更新
    (this->*spFuncTable_[static_cast<size_t>(step_)])();
}

///--------------------------------------------------------------------------
/// 描画
///--------------------------------------------------------------------------
void TitleAnimationManager::Draw() 
{
    titleBom_->Draw();
    titleBottom_->Draw();
}

void TitleAnimationManager::BackDraw() {
    for (size_t i = 0; i < backSprite_.size(); i++) {
    
        backSprite_[i]->Draw();
    }
}

///-------------------------------------------------------------------------
///  調節
///-------------------------------------------------------------------------
void TitleAnimationManager::AdjustParamater() 
{
    titleBom_->AdjustParm();
    titleBottom_->AdjustParm();

    if (Input::GetInstance()->TriggerKey(DIK_R)) {
        Reset();
    }
}

///-------------------------------------------------------------------------
///  ボムアニメーション
///-------------------------------------------------------------------------
void TitleAnimationManager::BomAnimation() 
{
    titleBom_->Update();
    if (!titleBom_->IsAnimationFinished())return;
    step_ = Step::BOTTOM;
}

///-------------------------------------------------------------------------
///  ボタンアニメーション
///-------------------------------------------------------------------------
void TitleAnimationManager::BottomAnimation() 
{
    titleBottom_->Update();
    if (!titleBottom_->IsAnimationFinished())return;
    step_ = Step::ALL;
}
///-------------------------------------------------------------------------
/// 全てアニメーション
///-------------------------------------------------------------------------
void TitleAnimationManager::AllAnimation() 
{
    titleBom_->ExPationing();
    titleBottom_->ExPationing();

    titleBom_->AdaptState();
    titleBottom_->AdaptState();
}

///-------------------------------------------------------------------------
///  ポインタテーブル
///-------------------------------------------------------------------------
void (TitleAnimationManager::* TitleAnimationManager::spFuncTable_[])()
{
    &TitleAnimationManager::BomAnimation,
    &TitleAnimationManager::BottomAnimation,
    &TitleAnimationManager::AllAnimation,
};

///-------------------------------------------------------------------------
///  リセット
///-------------------------------------------------------------------------
void TitleAnimationManager::Reset()
{
    titleBom_->Init();
    titleBottom_->Init();

    step_ = Step::BOM;
}