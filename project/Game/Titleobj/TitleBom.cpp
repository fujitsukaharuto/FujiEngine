#include "TitleBom.h"
#include "FPSKeeper.h"
#include "Sprite.h"
#include<imgui.h>
#include"ImGuiManager/ImGuiManager.h"
#include <memory>

TitleBom::TitleBom(){}

void TitleBom::Init()
{

    sprite_ = std::make_unique<Sprite>();
    sprite_->Load("GameTexture/Title_Logo.png");
    sprite_->SetAnchor(Vector2(0.5f, 0.5f));

    ///* グローバルパラメータ
    globalParameter_ = GlobalVariables::GetInstance();
    globalParameter_->CreateGroup(groupName_, false);
    AddParmGroup();
    ApplyGlobalParameter();

    kTextureWidth_ = 900.0f;
    kTextureHeigth_ = 700.0f;
    // pos
    positionY_ = paramater_.fallStartPosY_;
    //scale
    scale_={ paramater_.initScale_,paramater_.initScale_ };

    fallEaseTime_ = 0.0f;
    expationEaseTime_ = 0.0f;
    shrinkEaseTime_ = 0.0f;
    reverseEaseTime_ = 0.0f;
    waitTime_ = 0.0f;
    isFinished_ = false;
    step_ = Step::FALL;
  
};

void TitleBom::Update()
{
    // 終わってたら更新しない
    if (isFinished_) return;

    /// 関数ポインタ呼び出し
    (this->*spFuncTable_[static_cast<size_t>(step_)])();

    // サイズ、位置適応
    sprite_->SetPos(Vector3(paramater_.baseposX_, positionY_, 0));
    sprite_->SetSize(Vector2(kTextureWidth_* scale_.x, kTextureHeigth_ * scale_.y));

};

void TitleBom::Draw()
{
    sprite_->Draw();
};

///-------------------------------------------------------------------
///
///-------------------------------------------------------------------
void  TitleBom::Fall()
{
    if (FPSKeeper::DeltaTime() > 15.0f) {
        return;
    }
    // タイム加算
    fallEaseTime_ += FPSKeeper::NormalDeltaTime();

    // 適応
    positionY_ = EaseInCubic(paramater_.fallStartPosY_, paramater_.fallEndPosY_, fallEaseTime_, paramater_.fallEaseMaxTime);

    // 終わったらフラグとコールバック呼び出し
    if (fallEaseTime_ < paramater_.fallEaseMaxTime)return;
    positionY_ = paramater_.fallEndPosY_;
    step_ = Step::SHRINK;
    fallEaseTime_ = 0.0f;

}
///-------------------------------------------------------------------
///
///-------------------------------------------------------------------
void  TitleBom::Shrink()
{
    // タイム加算
    shrinkEaseTime_ += FPSKeeper::NormalDeltaTime();

    // 適応
    scale_.y = EaseOutExpo(paramater_.initScale_, paramater_.shrinkScale_, shrinkEaseTime_, paramater_.shrinkEaseMaxTime);

    // 終わったらフラグとコールバック呼び出し
    if (shrinkEaseTime_ < paramater_.shrinkEaseMaxTime)return;
    step_ = Step::EXPATION;
    shrinkEaseTime_ = 0.0f;
}
///-------------------------------------------------------------------
/// 
///-------------------------------------------------------------------
void  TitleBom::Expation()
{
    // タイム加算
    expationEaseTime_ += FPSKeeper::NormalDeltaTime();

    // 適応
    scale_.y = EaseInOutBack(paramater_.shrinkScale_, paramater_.expationScale_, expationEaseTime_, paramater_.expationMaxTime);
    scale_.x = EaseInOutBack(paramater_.initScale_, paramater_.expationScaleX_, expationEaseTime_, paramater_.expationMaxTime);

    // 終わったらフラグとコールバック呼び出し
    if (expationEaseTime_ < paramater_.expationMaxTime)return;
    step_ = Step::REVERSE;
    expationEaseTime_ = 0.0f;
}
void  TitleBom::Reverse()
{
    // タイム加算
    reverseEaseTime_ += FPSKeeper::NormalDeltaTime();

    // 適応
    scale_.y = EaseInOutBack(paramater_.expationScale_, paramater_.initScale_, reverseEaseTime_, paramater_.reverseMaxTime);
    scale_.x = EaseInOutBack(paramater_.expationScaleX_, paramater_.initScale_, reverseEaseTime_, paramater_.reverseMaxTime);

    // 終わったらフラグとコールバック呼び出し
    if (reverseEaseTime_ < paramater_.reverseMaxTime)return;
    scale_ = { paramater_.initScale_,paramater_.initScale_ };
    step_ = Step::WAIT;
    reverseEaseTime_ = 0.0f;
}
///-------------------------------------------------------------------
///
///-------------------------------------------------------------------
void  TitleBom::Wait()
{
    // 待機時間加算
    waitTime_ += FPSKeeper::NormalDeltaTime();

    // 待機終了
    if (waitTime_ < paramater_.kWaitTime_)return;
    isFinished_ = true;
}

bool TitleBom::IsAnimationFinished()
{
    return isFinished_;
};


void (TitleBom::* TitleBom::spFuncTable_[])()
{
    &TitleBom::Fall,
        & TitleBom::Shrink,
        & TitleBom::Expation,
        & TitleBom::Reverse,
        & TitleBom::Wait,
};

///=================================================================================
/// ロード
///=================================================================================
void TitleBom::ParmLoadForImGui() {

    // ロードボタン
    if (ImGui::Button(std::format("Load {}", groupName_).c_str())) {

        globalParameter_->LoadFile(groupName_);
        // セーブ完了メッセージ
        ImGui::Text("Load Successful: %s", groupName_.c_str());
        ApplyGlobalParameter();
    }
}


///=================================================================================
///パラメータをグループに追加
///=================================================================================
void TitleBom::AddParmGroup() {

 
    globalParameter_->AddItem(groupName_, "fallEaseMaxTime", paramater_.fallEaseMaxTime);
    globalParameter_->AddItem(groupName_, "maxTime", paramater_.expationMaxTime);
    globalParameter_->AddItem(groupName_, "shrinkEaseMaxTime", paramater_.shrinkEaseMaxTime);
    globalParameter_->AddItem(groupName_, "reverseMaxTime", paramater_.reverseMaxTime);
    globalParameter_->AddItem(groupName_, "kWaitTime_", paramater_.kWaitTime_);
    globalParameter_->AddItem(groupName_, "fallStartPosY_", paramater_.fallStartPosY_);
    globalParameter_->AddItem(groupName_, "fallEndPosY_", paramater_.fallEndPosY_);
    globalParameter_->AddItem(groupName_, "initScale_", paramater_.initScale_);
    globalParameter_->AddItem(groupName_, "expationScale_", paramater_.expationScale_);
    globalParameter_->AddItem(groupName_, "shrinkScale_", paramater_.shrinkScale_);
    globalParameter_->AddItem(groupName_, "baseposX_", paramater_.baseposX_);
    globalParameter_->AddItem(groupName_, "expationScaleX_", paramater_.expationScaleX_);
}


///=================================================================================
///パラメータをグループに追加
///=================================================================================
void TitleBom::SetValues() {

    globalParameter_->SetValue(groupName_, "fallEaseMaxTime", paramater_.fallEaseMaxTime);
    globalParameter_->SetValue(groupName_, "maxTime", paramater_.expationMaxTime);
    globalParameter_->SetValue(groupName_, "shrinkEaseMaxTime", paramater_.shrinkEaseMaxTime);
    globalParameter_->SetValue(groupName_, "reverseMaxTime", paramater_.reverseMaxTime);
    globalParameter_->SetValue(groupName_, "kWaitTime_", paramater_.kWaitTime_);
    globalParameter_->SetValue(groupName_, "fallStartPosY_", paramater_.fallStartPosY_);
    globalParameter_->SetValue(groupName_, "fallEndPosY_", paramater_.fallEndPosY_);
    globalParameter_->SetValue(groupName_, "initScale_", paramater_.initScale_);
    globalParameter_->SetValue(groupName_, "expationScale_", paramater_.expationScale_);
    globalParameter_->SetValue(groupName_, "shrinkScale_", paramater_.shrinkScale_);
    globalParameter_->SetValue(groupName_, "baseposX_", paramater_.baseposX_);
    globalParameter_->SetValue(groupName_, "expationScaleX_", paramater_.expationScaleX_);
}


///=====================================================
///  ImGuiからパラメータを得る
///===================================================== 
void TitleBom::ApplyGlobalParameter() {

    paramater_.fallEaseMaxTime = globalParameter_->GetValue<float>(groupName_, "fallEaseMaxTime");
    paramater_.expationMaxTime = globalParameter_->GetValue<float>(groupName_, "maxTime");
    paramater_.shrinkEaseMaxTime = globalParameter_->GetValue<float>(groupName_, "shrinkEaseMaxTime");
    paramater_.reverseMaxTime = globalParameter_->GetValue<float>(groupName_, "reverseMaxTime");
    paramater_.kWaitTime_ = globalParameter_->GetValue<float>(groupName_, "kWaitTime_");
    paramater_.fallStartPosY_ = globalParameter_->GetValue<float>(groupName_, "fallStartPosY_");
    paramater_.fallEndPosY_ = globalParameter_->GetValue<float>(groupName_, "fallEndPosY_");
    paramater_.initScale_ = globalParameter_->GetValue<float>(groupName_, "initScale_");
    paramater_.expationScale_ = globalParameter_->GetValue<float>(groupName_, "expationScale_");
    paramater_.shrinkScale_ = globalParameter_->GetValue<float>(groupName_, "shrinkScale_");
    paramater_.baseposX_ = globalParameter_->GetValue<float>(groupName_, "baseposX_");
    paramater_.expationScaleX_ = globalParameter_->GetValue<float>(groupName_, "expationScaleX_");
}

///=========================================================
/// パラメータ調整
///==========================================================
void TitleBom::AdjustParm() {
    SetValues();
#ifdef _DEBUG

    if (ImGui::CollapsingHeader(groupName_.c_str())) {
        ImGui::PushID(groupName_.c_str());
        ImGuiManager::GetInstance()->SetFontJapanese();/// 日本語

        ImGui::DragFloat("基準位置X", &paramater_.baseposX_, 0.1f);
        ImGui::DragFloat("落下イージング最大時間", &paramater_.fallEaseMaxTime, 0.01f);
        ImGui::DragFloat("最大拡大時間", &paramater_.expationMaxTime, 0.01f);
        ImGui::DragFloat("縮小イージング最大時間", &paramater_.shrinkEaseMaxTime, 0.01f);
        ImGui::DragFloat("反転最大時間", &paramater_.reverseMaxTime, 0.01f);
        ImGui::DragFloat("待機時間", &paramater_.kWaitTime_, 0.01f);
        ImGui::DragFloat("落下開始位置Y", &paramater_.fallStartPosY_, 0.1f);
        ImGui::DragFloat("落下終了位置Y", &paramater_.fallEndPosY_, 0.1f);
        ImGui::DragFloat("初期スケール", &paramater_.initScale_, 0.01f);
        ImGui::DragFloat("拡大スケール", &paramater_.expationScale_, 0.01f);
        ImGui::DragFloat("拡大スケールX", &paramater_.expationScaleX_, 0.01f);
        ImGui::DragFloat("縮小スケール", &paramater_.shrinkScale_, 0.01f);

        globalParameter_->ParmSaveForImGui(groupName_);
        ParmLoadForImGui();
        ImGuiManager::GetInstance()->UnSetFont();
        ImGui::PopID();
    }

#endif // _DEBUG
}