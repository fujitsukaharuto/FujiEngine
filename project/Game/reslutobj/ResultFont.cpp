#include "ResultFont.h"
#include "FPSKeeper.h"
#include "Sprite.h"
#include<imgui.h>
#include"ImGuiManager/ImGuiManager.h"
#include <memory>

ResultFont::ResultFont(){
}

void ResultFont::Init()
{
   
    sprite_ = std::make_unique<Sprite>();
    sprite_->Load("GameTexture/Clear.png");
    sprite_->SetAnchor(Vector2(0.5f, 0.5f));

    ///* グローバルパラメータ
    globalParameter_ = GlobalVariables::GetInstance();
    globalParameter_->CreateGroup(groupName_, false);
    AddParmGroup();
    ApplyGlobalParameter();

    /// リセット
    Reset();
  
};

void ResultFont::Update()
{
  
    // 終わってたら更新しない
    if (isFinished_) return;

    /// 関数ポインタ呼び出し
    (this->*spFuncTable_[static_cast<size_t>(step_)])();

    AdaptState();
};

void ResultFont::AdaptState() {
    // サイズ、位置適応
    sprite_->SetPos(Vector3(560.0f, positionY_, 0));
    sprite_->SetSize(Vector2(kTextureWidth_ , kTextureHeigth_));

}

void ResultFont::Draw()
{
    sprite_->Draw();
};

///-------------------------------------------------------------------
///
///-------------------------------------------------------------------
void  ResultFont::Fall()
{
    if (FPSKeeper::DeltaTime() > 15.0f) {
        return;
    }
    // タイム加算
    fallEaseTime_ += FPSKeeper::NormalDeltaTime();

    // 適応
    positionY_ = EaseOutBack(paramater_.fallStartPosY_, paramater_.fallEndPosY_, fallEaseTime_, paramater_.fallEaseMaxTime);

    // 終わったらフラグとコールバック呼び出し
    if (fallEaseTime_ < paramater_.fallEaseMaxTime)return;
    positionY_ = paramater_.fallEndPosY_;
    step_ = Step::WAIT;
    fallEaseTime_ = 0.0f;

}

///-------------------------------------------------------------------
///　待機
///-------------------------------------------------------------------
void  ResultFont::Wait()
{
    // 待機時間加算
    waitTime_ += FPSKeeper::NormalDeltaTime();

    // 待機終了
    if (waitTime_ < paramater_.kWaitTime_)return;
    isFinished_ = true;
}

bool ResultFont::IsAnimationFinished()
{
    return isFinished_;
};


void (ResultFont::* ResultFont::spFuncTable_[])()
{
        &ResultFont::Fall,
        &ResultFont::Wait,
};

///=================================================================================
/// ロード
///=================================================================================
void ResultFont::ParmLoadForImGui() {

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
void ResultFont::AddParmGroup() {

 
    globalParameter_->AddItem(groupName_, "fallEaseMaxTime", paramater_.fallEaseMaxTime);
    globalParameter_->AddItem(groupName_, "kWaitTime_", paramater_.kWaitTime_);
    globalParameter_->AddItem(groupName_, "fallStartPosY_", paramater_.fallStartPosY_);
    globalParameter_->AddItem(groupName_, "fallEndPosY_", paramater_.fallEndPosY_);
   
}


///=================================================================================
///パラメータをグループに追加
///=================================================================================
void ResultFont::SetValues() {

    globalParameter_->SetValue(groupName_, "fallEaseMaxTime", paramater_.fallEaseMaxTime);
    globalParameter_->SetValue(groupName_, "kWaitTime_", paramater_.kWaitTime_);
    globalParameter_->SetValue(groupName_, "fallStartPosY_", paramater_.fallStartPosY_);
    globalParameter_->SetValue(groupName_, "fallEndPosY_", paramater_.fallEndPosY_);
   
}


///=====================================================
///  ImGuiからパラメータを得る
///===================================================== 
void ResultFont::ApplyGlobalParameter() {

    paramater_.fallEaseMaxTime = globalParameter_->GetValue<float>(groupName_, "fallEaseMaxTime");
    paramater_.kWaitTime_ = globalParameter_->GetValue<float>(groupName_, "kWaitTime_");
    paramater_.fallStartPosY_ = globalParameter_->GetValue<float>(groupName_, "fallStartPosY_");
    paramater_.fallEndPosY_ = globalParameter_->GetValue<float>(groupName_, "fallEndPosY_");
  
}

///=========================================================
/// パラメータ調整
///==========================================================
void ResultFont::AdjustParm() {
    SetValues();
#ifdef _DEBUG

    if (ImGui::CollapsingHeader(groupName_.c_str())) {
        ImGui::PushID(groupName_.c_str());
        ImGuiManager::GetInstance()->SetFontJapanese();/// 日本語

     
        ImGui::DragFloat("落下イージング最大時間", &paramater_.fallEaseMaxTime, 0.01f);
        ImGui::DragFloat("待機時間", &paramater_.kWaitTime_, 0.01f);
        ImGui::DragFloat("落下開始位置Y", &paramater_.fallStartPosY_, 0.1f);
        ImGui::DragFloat("落下終了位置Y", &paramater_.fallEndPosY_, 0.1f);
   

        globalParameter_->ParmSaveForImGui(groupName_);
        ParmLoadForImGui();
        ImGuiManager::GetInstance()->UnSetFont();
        ImGui::PopID();
    }

#endif // _DEBUG
}

void ResultFont::Reset() {
    /// parm
    kTextureWidth_ = 850.0f;
    kTextureHeigth_ = 200.0f;
    positionY_ = paramater_.fallStartPosY_; // pos
    easeDirection_ = 1.0f;

    fallEaseTime_ = 0.0f;
    waitTime_ = 0.0f;
    isFinished_ = false;
    step_ = Step::FALL;
}

void ResultFont::ExPationing() {
    // 具体的な処理が分からないので、適当な実装
    // 何もする必要がなければ空の関数でもOK
}