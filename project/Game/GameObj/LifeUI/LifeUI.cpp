#include"LifeUI.h"
#include"GameObj/Player/Player.h"
#include"Random.h"
/// behavior
#include"behavior/LifeUIRoot.h"
#include"DX/FPSKeeper.h"
#include"ImGuiManager.h"
#include<imgui.h>

// 初期化
void LifeUI::Init() {
    life_ = lifeMax_;
    isShakeStart_ = false;

	///* グローバルパラメータ
	globalParameter_ = GlobalVariables::GetInstance();
	globalParameter_->CreateGroup(groupName_, false);
	AddParmGroup();
	ApplyGlobalParameter();

	// 初期化
	for (int i = 0; i < sprites_.size(); i++) {
		sprites_[i] = std::make_unique<Sprite>();
		sprites_[i]->Load("GameTexture/Life.png");
		sprites_[i]->SetColor  ({ 1.0f,1.0f,1.0f,1.0f });
		sprites_[i]->SetAnchor ({ 0.0f,0.0f });
		sprites_[i]->SetSize({ paramater_.kTextureWidth,paramater_.kTextureHeight }); // サイズを固定
	}

    // パラメータ初期化
    sizeWidth_  = paramater_.kTextureWidth;
    sizeHeigth_ = paramater_.kTextureHeight;
    offset_     = paramater_.offsetNormal;
    basePos_    = paramater_.basePosies_[0];

    isGameOver_ = false;
    shakeValue_.x = 0.0f;
    shakeValue_.y = 0.0f;


    // 振る舞い初期化
    ChangeBehavior(std::make_unique<LifeUIRoot>(this));
}

// 更新
void LifeUI::Update() {

    Shake();
   /* basePos_ = paramater_.basePosies_[2];*/

    // 位置、サイズ
    for (int i = 0; i < sprites_.size(); i++) {
        sprites_[i]->SetPos ({ basePos_.x + (float(i) * offset_)+ shakeValue_.x, basePos_.y+ shakeValue_.y, basePos_.z });
        sprites_[i]->SetSize({ sizeWidth_,sizeHeigth_ }); // サイズを固定
    }

    /// behavior
    behavior_->Update();
}


// 描画
void LifeUI::Draw() {
	/// 描画
	for (int i = 0; i < sprites_.size(); i++) {
		sprites_[i]->Draw();
	}
}

///=================================================================================
/// ロード
///=================================================================================
void LifeUI::ParmLoadForImGui() {

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
void LifeUI::AddParmGroup() {

    // Position
    globalParameter_->AddItem(groupName_, "basePosNormal", paramater_.basePosies_[0]);
    globalParameter_->AddItem(groupName_, "basePosNormal1", paramater_.basePosies_[1]);
    globalParameter_->AddItem(groupName_, "basePosNormal2", paramater_.basePosies_[2]);
    globalParameter_->AddItem(groupName_, "basePosDeath", paramater_.basePosiesDeath[0]);
    globalParameter_->AddItem(groupName_, "basePosDeath1", paramater_.basePosiesDeath[1]);
    globalParameter_->AddItem(groupName_, "basePosDeath2", paramater_.basePosiesDeath[2]);
    globalParameter_->AddItem(groupName_, "offsetNormal", paramater_.offsetNormal);
    globalParameter_->AddItem(groupName_, "offsetDeath", paramater_.offsetDeath);
    globalParameter_->AddItem(groupName_, "moveUIEaseTime_", paramater_.moveUIEaseTime_);
    globalParameter_->AddItem(groupName_, "deathTextureWidth_", paramater_.deathTextureWidth_);
    globalParameter_->AddItem(groupName_, "deathTextureHeigth_", paramater_.deathTextureHeigth_);
    globalParameter_->AddItem(groupName_, "moveWaitTime_", paramater_.moveWaitTime_);
    globalParameter_->AddItem(groupName_, "braekWaitTime_", paramater_.braekWaitTime_);
    globalParameter_->AddItem(groupName_, "emitterPosies_[0]", paramater_.emitterPosies_[0]);
    globalParameter_->AddItem(groupName_, "emitterPosies_[1]", paramater_.emitterPosies_[1]);
    globalParameter_->AddItem(groupName_, "emitterPosies_[2]", paramater_.emitterPosies_[2]);
}


///===================================================================================
///パラメータをグループに追加
///===================================================================================
void LifeUI::SetValues() {
    globalParameter_->SetValue(groupName_, "basePosNormal", paramater_.basePosies_[0]);
    globalParameter_->SetValue(groupName_, "basePosNormal1", paramater_.basePosies_[1]);
    globalParameter_->SetValue(groupName_, "basePosNormal2", paramater_.basePosies_[2]);
    globalParameter_->SetValue(groupName_, "basePosDeath", paramater_.basePosiesDeath[0]);
    globalParameter_->SetValue(groupName_, "basePosDeath1", paramater_.basePosiesDeath[1]);
    globalParameter_->SetValue(groupName_, "basePosDeath2", paramater_.basePosiesDeath[2]);
    globalParameter_->SetValue(groupName_, "offsetNormal", paramater_.offsetNormal);
    globalParameter_->SetValue(groupName_, "offsetDeath", paramater_.offsetDeath);
    globalParameter_->SetValue(groupName_, "moveUIEaseTime_", paramater_.moveUIEaseTime_);
    globalParameter_->SetValue(groupName_, "deathTextureWidth_", paramater_.deathTextureWidth_);
    globalParameter_->SetValue(groupName_, "deathTextureHeigth_", paramater_.deathTextureHeigth_);
    globalParameter_->SetValue(groupName_, "moveWaitTime_", paramater_.moveWaitTime_);
    globalParameter_->SetValue(groupName_, "braekWaitTime_", paramater_.braekWaitTime_);
    globalParameter_->SetValue(groupName_, "emitterPosies_[0]", paramater_.emitterPosies_[0]);
    globalParameter_->SetValue(groupName_, "emitterPosies_[1]", paramater_.emitterPosies_[1]);
    globalParameter_->SetValue(groupName_, "emitterPosies_[2]", paramater_.emitterPosies_[2]);
}


///=====================================================
///  ImGuiからパラメータを得る
///===================================================== 
void LifeUI::ApplyGlobalParameter() {
    // Position
    paramater_.offsetNormal=globalParameter_->GetValue<float>(groupName_, "offsetNormal");
    paramater_.offsetDeath = globalParameter_->GetValue<float>(groupName_, "offsetDeath");
    paramater_.moveUIEaseTime_ = globalParameter_->GetValue<float>(groupName_, "moveUIEaseTime_");
    paramater_.deathTextureWidth_ = globalParameter_->GetValue<float>(groupName_, "deathTextureWidth_");
    paramater_.deathTextureHeigth_ = globalParameter_->GetValue<float>(groupName_, "deathTextureHeigth_");
    paramater_.moveWaitTime_ = globalParameter_->GetValue<float>(groupName_, "moveWaitTime_");
    paramater_.braekWaitTime_ = globalParameter_->GetValue<float>(groupName_, "braekWaitTime_");
    paramater_.emitterPosies_[0] = globalParameter_->GetValue<Vector3>(groupName_, "emitterPosies_[0]");
    paramater_.emitterPosies_[1] = globalParameter_->GetValue<Vector3>(groupName_, "emitterPosies_[1]");
    paramater_.emitterPosies_[2] = globalParameter_->GetValue<Vector3>(groupName_, "emitterPosies_[2]");
    paramater_.basePosies_[0] = globalParameter_->GetValue<Vector3>(groupName_, "basePosNormal");
    paramater_.basePosies_[1] = globalParameter_->GetValue<Vector3>(groupName_, "basePosNormal1");
    paramater_.basePosies_[2] = globalParameter_->GetValue<Vector3>(groupName_, "basePosNormal2");
    paramater_.basePosiesDeath[0] = globalParameter_->GetValue<Vector3>(groupName_, "basePosDeath");
    paramater_.basePosiesDeath[1] = globalParameter_->GetValue<Vector3>(groupName_, "basePosDeath1");
    paramater_.basePosiesDeath[2] = globalParameter_->GetValue<Vector3>(groupName_, "basePosDeath2");
}

///=========================================================
/// パラメータ調整
///==========================================================
void LifeUI::AdjustParm() {
    SetValues();
#ifdef _DEBUG
   
    if (ImGui::CollapsingHeader(groupName_.c_str())) {
        ImGui::PushID(groupName_.c_str());
        ImGuiManager::GetInstance()->SetFontJapanese();/// 日本語

        ImGui::SeparatorText("位置");
        ImGui::DragFloat3("通常時の位置1", &paramater_.basePosies_[0].x, 0.1f);
        ImGui::DragFloat3("通常時の位置2", &paramater_.basePosies_[1].x, 0.1f);
        ImGui::DragFloat3("通常時の位置3", &paramater_.basePosies_[2].x, 0.1f);
        ImGui::DragFloat3("死亡時の位置1", &paramater_.basePosiesDeath[0].x, 0.1f);
        ImGui::DragFloat3("死亡時の位置2", &paramater_.basePosiesDeath[1].x, 0.1f);
        ImGui::DragFloat3("死亡時の位置3", &paramater_.basePosiesDeath[2].x, 0.1f);
   
        ImGui::DragFloat("通常時のUI間隔", &paramater_.offsetNormal, 0.1f);
        ImGui::DragFloat("死亡時のUI間隔", &paramater_.offsetDeath, 0.1f);

        ImGui::DragFloat3("エミッター位置1", &paramater_.emitterPosies_[0].x, 0.1f);
        ImGui::DragFloat3("エミッター位置2", &paramater_.emitterPosies_[1].x, 0.1f);
        ImGui::DragFloat3("エミッター位置3", &paramater_.emitterPosies_[2].x, 0.1f);
        ImGui::SeparatorText("サイズ");
        ImGui::DragFloat("死亡時のサイズ横", &paramater_.deathTextureWidth_, 0.1f);
        ImGui::DragFloat("死亡時のサイズ縦", &paramater_.deathTextureHeigth_, 0.1f);
        ImGui::SeparatorText("イージング");
        ImGui::DragFloat("開始までの待機時間", &paramater_.moveWaitTime_, 0.01f);
        ImGui::DragFloat("破壊までの待機時間", &paramater_.braekWaitTime_, 0.01f);
        ImGui::DragFloat("UIが動くイージングタイム", &paramater_.moveUIEaseTime_, 0.01f);
        /// セーブとロード
        globalParameter_->ParmSaveForImGui(groupName_);
        ParmLoadForImGui();

        ImGuiManager::GetInstance()->UnSetFont();
        ImGui::PopID();
    }

#endif // _DEBUG
}

void LifeUI::LifeBreak() {
    if (life_ > 0 && !sprites_.empty()) {
        size_t index = lifeMax_ - life_; // 現在のライフに対応するスプライト

        if (index < sprites_.size()) {
         
            // サイズを0にする（非表示にする）
            emitterPos_ = paramater_.emitterPosies_[index];
            sprites_[index]->SetColor(Vector4(0,0,0,0));

            // ライフを減らす
            life_--;

            if (life_ > 1)return;
            isShakeStart_ = true;
        }
    }
}

void LifeUI::SetPlayer(Player* player) {
    pPlayer_ = player;
}


void  LifeUI::ChangeBehavior(std::unique_ptr<BaseLifeUIBehavior> behavior) {
    behavior_ = std::move(behavior);
}


void LifeUI::Reset() {

}

void LifeUI::Shake() {
    if (!isShakeStart_) {
        shakeValue_ = { 0.0f, 0.0f}; // シェイク終了後、値をリセット
        return;
    }

    // ランダムなオフセットを生成
    shakeValue_.x = Random::GetFloat(-10, 10) * 0.4f;
    shakeValue_.y = Random::GetFloat(-10, 10) * 0.4f;

    //shakeValue_ = { 0.0f, 0.0f }; // シェイク終了後、値をリセット

}
