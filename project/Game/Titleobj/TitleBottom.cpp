#include "TitleBottom.h"
#include "FPSKeeper.h"
#include "Sprite.h"
#include<imgui.h>
#include"ImGuiManager/ImGuiManager.h"
#include <memory>

TitleBottom::TitleBottom(){}

void TitleBottom::Init()
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

	//time
	paramater_.reverseMaxTime = 0.5f;
	//scale
	paramater_.initScale_ = 1.0f;
	paramater_.expationScale_ = 1.5f;
	scalerScale_ = paramater_.initScale_;
	isFinished_ = false;
	step_ = Step::EXPATION;
	expationEaseTime_ = 0.0f;
	reverseEaseTime_ = 0.0f;
};

void TitleBottom::Update()
{
	// 終わってたら更新しない
	if (isFinished_) return;

	/// 関数ポインタ呼び出し
	(this->*spFuncTable_[static_cast<size_t>(step_)])();

	// サイズ、位置適応
	sprite_->SetSize(Vector2(kTextureWidth_ * scalerScale_, kTextureHeigth_ * scalerScale_));

};

void TitleBottom::Draw()
{
	sprite_->Draw();
};


///-------------------------------------------------------------------
/// 
///-------------------------------------------------------------------
void  TitleBottom::Expation()
{
	// タイム加算
	expationEaseTime_ += FPSKeeper::NormalDeltaTime();

	// 適応
	scalerScale_ = EaseInOutBack(paramater_.initScale_, paramater_.expationScale_, expationEaseTime_, paramater_.expationMaxTime);

	// 終わったらフラグとコールバック呼び出し
	if (expationEaseTime_ < paramater_.expationMaxTime)return;
	step_ = Step::REVERSE;
	scalerScale_ = paramater_.expationScale_;
	expationEaseTime_ = 0.0f;
}
void  TitleBottom::Reverse()
{
	// タイム加算
	reverseEaseTime_ += FPSKeeper::NormalDeltaTime();

	// 適応
	scalerScale_ = EaseInOutBack(paramater_.expationScale_, paramater_.initScale_, reverseEaseTime_, paramater_.reverseMaxTime);

	// 終わったらフラグとコールバック呼び出し
	if (reverseEaseTime_ < paramater_.reverseMaxTime)return;
	scalerScale_ = paramater_.initScale_;
	step_ = Step::WAIT;
	reverseEaseTime_ = 0.0f;
}
///-------------------------------------------------------------------
///
///-------------------------------------------------------------------
void   TitleBottom::Wait()
{
	// 待機時間加算
	waitTime_ += FPSKeeper::NormalDeltaTime();

	// 待機終了
	if (waitTime_ < paramater_.kWaitTime)return;
	isFinished_ = true;
	

}

bool TitleBottom::IsAnimationFinished()
{
	return isFinished_;
};


void (TitleBottom::* TitleBottom::spFuncTable_[])()
{
	&TitleBottom::Expation,
	&TitleBottom::Reverse,
	&TitleBottom::Wait,
};

///=================================================================================
/// ロード
///=================================================================================
void TitleBottom::ParmLoadForImGui() {

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
void TitleBottom::AddParmGroup() {

	globalParameter_->AddItem(groupName_, "reverseMaxTime", paramater_.reverseMaxTime);
	globalParameter_->AddItem(groupName_, "initScale_", paramater_.initScale_);
	globalParameter_->AddItem(groupName_, "expationScale_", paramater_.expationScale_);
	globalParameter_->AddItem(groupName_, "kWaitTime", paramater_.kWaitTime);
}


///=================================================================================
///パラメータをグループに追加
///=================================================================================
void TitleBottom::SetValues() {


	globalParameter_->SetValue(groupName_, "reverseMaxTime", paramater_.reverseMaxTime);
	globalParameter_->SetValue(groupName_, "initScale_", paramater_.initScale_);
	globalParameter_->SetValue(groupName_, "expationScale_", paramater_.expationScale_);
	globalParameter_->SetValue(groupName_, "kWaitTime", paramater_.kWaitTime);
}


///=====================================================
///  ImGuiからパラメータを得る
///===================================================== 
void TitleBottom::ApplyGlobalParameter() {


	paramater_.reverseMaxTime = globalParameter_->GetValue<float>(groupName_, "reverseMaxTime");
	paramater_.initScale_ = globalParameter_->GetValue<float>(groupName_, "initScale_");
	paramater_.expationScale_ = globalParameter_->GetValue<float>(groupName_, "expationScale_");
	paramater_.kWaitTime = globalParameter_->GetValue<float>(groupName_, "kWaitTime");
}

///=========================================================
/// パラメータ調整
///==========================================================
void TitleBottom::AdjustParm() {
	SetValues();
#ifdef _DEBUG

	if (ImGui::CollapsingHeader(groupName_.c_str())) {
		ImGui::PushID(groupName_.c_str());
		ImGuiManager::GetInstance()->SetFontJapanese();/// 日本語

		ImGui::DragFloat("reverseMaxTime", &paramater_.reverseMaxTime, 0.01f);
		ImGui::DragFloat("initScale_", &paramater_.initScale_, 0.01f);
		ImGui::DragFloat("expationScale_", &paramater_.expationScale_, 0.01f);
		ImGui::DragFloat("kWaitTime", &paramater_.kWaitTime, 0.01f);

		globalParameter_->ParmSaveForImGui(groupName_);
		ParmLoadForImGui();
		ImGuiManager::GetInstance()->UnSetFont();
		ImGui::PopID();
	}

#endif // _DEBUG
}