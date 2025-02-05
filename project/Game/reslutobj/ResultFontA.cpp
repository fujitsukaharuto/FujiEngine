#include "ResultFontA.h"
#include "FPSKeeper.h"
#include "Sprite.h"
#include<imgui.h>
#include"ImGuiManager/ImGuiManager.h"
#include <memory>

ResultFontA::ResultFontA(){}

void ResultFontA::Init()
{
	// ボタン
	sprite_ = std::make_unique<Sprite>();
	sprite_->Load("GameTexture/Clear_Key.png");
	sprite_->SetAnchor(Vector2(0.5f, 0.5f));

	///* グローバルパラメータ
	globalParameter_ = GlobalVariables::GetInstance();
	globalParameter_->CreateGroup(groupName_, false);
	AddParmGroup();
	ApplyGlobalParameter();

	// リセット
	Reset();
	
};

void ResultFontA::Update()
{

	// 終わってたら更新しない
	if (isFinished_) return;

	/// 関数ポインタ呼び出し
	(this->*spFuncTable_[static_cast<size_t>(step_)])();

	AdaptState();

};

void ResultFontA::AdaptState() {
	// サイズ、位置適応
	sprite_->SetSize(Vector2(paramater_.bottomAWidth * scalerScale_, paramater_.bottomAHeigth * scalerScale_));
	
	/// スプライト
	sprite_->SetPos(Vector3(paramater_.bottomAPos_));

}

void ResultFontA::Draw()
{
	sprite_->Draw();
	
};


///-------------------------------------------------------------------
/// 
///-------------------------------------------------------------------
void  ResultFontA::Expation()
{
	// タイム加算
	expationEaseTime_ += FPSKeeper::NormalDeltaTime();

	// 適応
	scalerScale_ = EaseInCubic(paramater_.initScale_, paramater_.expationScale_, expationEaseTime_, paramater_.expationMaxTime);

	// 終わったらフラグとコールバック呼び出し
	if (expationEaseTime_ < paramater_.expationMaxTime)return;
	step_ = Step::REVERSE;
	scalerScale_ = paramater_.expationScale_;
	expationEaseTime_ = 0.0f;
}
void  ResultFontA::Reverse()
{
	// タイム加算
	reverseEaseTime_ += FPSKeeper::NormalDeltaTime();

	// 適応
	scalerScale_ = EaseInCubic(paramater_.expationScale_, 1.0f, reverseEaseTime_, paramater_.reverseMaxTime);

	// 終わったらフラグとコールバック呼び出し
	if (reverseEaseTime_ < paramater_.reverseMaxTime)return;
	scalerScale_ = 1.0f;
	reverseEaseTime_ = 0.0f;
	step_ = Step::WAIT;
}
///-------------------------------------------------------------------
///
///-------------------------------------------------------------------
void   ResultFontA::Wait()
{
	// 待機時間加算
	waitTime_ += FPSKeeper::NormalDeltaTime();

	// 待機終了
	if (waitTime_ < paramater_.kWaitTime)return;
	isFinished_ = true;

}

void ResultFontA::ExPationing()
{
	// イージングタイムを更新
	scalingEaseTime_ += FPSKeeper::NormalDeltaTime() * easeDirection_; // 方向に応じて時間を増減

	// タイムが1を超えたら逆方向に、0未満になったら進む方向に変更
	if (scalingEaseTime_ >= paramater_.scalingEaseTMax) {
		scalingEaseTime_ = paramater_.scalingEaseTMax;
		easeDirection_ = -1.0f; // 逆方向に切り替え
	}
	else if (scalingEaseTime_ <= 0.0f) {
		scalingEaseTime_ = 0.0f;
		easeDirection_ = 1.0f; // 進む方向に切り替え

	}

	scalerScale_ = EaseInCubic(1.0f, paramater_.expatingScale, scalingEaseTime_, paramater_.scalingEaseTMax);
}

bool ResultFontA::IsAnimationFinished()
{
	return isFinished_;
};


void (ResultFontA::* ResultFontA::spFuncTable_[])()
{
	&ResultFontA::Expation,
	&ResultFontA::Reverse,
	&ResultFontA::Wait,
};

///=================================================================================
/// ロード
///=================================================================================
void ResultFontA::ParmLoadForImGui() {

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
void ResultFontA::AddParmGroup() {

	globalParameter_->AddItem(groupName_, "bottomAPos", paramater_.bottomAPos_);
	globalParameter_->AddItem(groupName_, "controllaPos", paramater_.controllaPos_);
	globalParameter_->AddItem(groupName_, "reverseMaxTime", paramater_.reverseMaxTime);
	globalParameter_->AddItem(groupName_, "expationMaxTime", paramater_.expationMaxTime);
	globalParameter_->AddItem(groupName_, "initScale_", paramater_.initScale_);
	globalParameter_->AddItem(groupName_, "expationScale_", paramater_.expationScale_);
	globalParameter_->AddItem(groupName_, "kWaitTime", paramater_.kWaitTime);
	globalParameter_->AddItem(groupName_, "bottomAWidth", paramater_.bottomAWidth);
	globalParameter_->AddItem(groupName_, "bottomAHeigth", paramater_.bottomAHeigth);
	globalParameter_->AddItem(groupName_, "controllaWidth", paramater_.controllaWidth);
	globalParameter_->AddItem(groupName_, "controllaHeigth", paramater_.controllaHeigth);
	globalParameter_->AddItem(groupName_, "scalingEaseTMax", paramater_.scalingEaseTMax);
	globalParameter_->AddItem(groupName_, "expatingScale", paramater_.expatingScale);
}

///=================================================================================
///パラメータをグループに追加
///=================================================================================
void ResultFontA::SetValues() {

	globalParameter_->SetValue(groupName_, "bottomAPos", paramater_.bottomAPos_);
	globalParameter_->SetValue(groupName_, "controllaPos", paramater_.controllaPos_);
	globalParameter_->SetValue(groupName_, "reverseMaxTime", paramater_.reverseMaxTime);
	globalParameter_->SetValue(groupName_, "expationMaxTime", paramater_.expationMaxTime);
	globalParameter_->SetValue(groupName_, "initScale_", paramater_.initScale_);
	globalParameter_->SetValue(groupName_, "expationScale_", paramater_.expationScale_);
	globalParameter_->SetValue(groupName_, "kWaitTime", paramater_.kWaitTime);
	globalParameter_->SetValue(groupName_, "bottomAWidth", paramater_.bottomAWidth);
	globalParameter_->SetValue(groupName_, "bottomAHeigth", paramater_.bottomAHeigth);
	globalParameter_->SetValue(groupName_, "controllaWidth", paramater_.controllaWidth);
	globalParameter_->SetValue(groupName_, "controllaHeigth", paramater_.controllaHeigth);
	globalParameter_->SetValue(groupName_, "scalingEaseTMax", paramater_.scalingEaseTMax);
	globalParameter_->SetValue(groupName_, "expatingScale", paramater_.expatingScale);
}


///=====================================================
///  ImGuiからパラメータを得る
///===================================================== 
void ResultFontA::ApplyGlobalParameter() {


	paramater_.bottomAPos_ = globalParameter_->GetValue<Vector3>(groupName_, "bottomAPos");
	paramater_.controllaPos_ = globalParameter_->GetValue<Vector3>(groupName_, "controllaPos");
	paramater_.reverseMaxTime = globalParameter_->GetValue<float>(groupName_, "reverseMaxTime");
	paramater_.expationMaxTime = globalParameter_->GetValue<float>(groupName_, "expationMaxTime");
	paramater_.initScale_ = globalParameter_->GetValue<float>(groupName_, "initScale_");
	paramater_.expationScale_ = globalParameter_->GetValue<float>(groupName_, "expationScale_");
	paramater_.kWaitTime = globalParameter_->GetValue<float>(groupName_, "kWaitTime");
	paramater_.bottomAWidth = globalParameter_->GetValue<float>(groupName_, "bottomAWidth");
	paramater_.bottomAHeigth = globalParameter_->GetValue<float>(groupName_, "bottomAHeigth");
	paramater_.controllaWidth = globalParameter_->GetValue<float>(groupName_, "controllaWidth");
	paramater_.controllaHeigth = globalParameter_->GetValue<float>(groupName_, "controllaHeigth");
	paramater_.scalingEaseTMax = globalParameter_->GetValue<float>(groupName_, "scalingEaseTMax");
	paramater_.expatingScale = globalParameter_->GetValue<float>(groupName_, "expatingScale");
}

///=========================================================
/// パラメータ調整
///==========================================================
void ResultFontA::AdjustParm() {
	SetValues();
#ifdef _DEBUG

	if (ImGui::CollapsingHeader(groupName_.c_str())) {
		ImGui::PushID(groupName_.c_str());
		ImGuiManager::GetInstance()->SetFontJapanese();/// 日本語

		ImGui::DragFloat3("bottomAPos", &paramater_.bottomAPos_.x, 0.1f);
		ImGui::DragFloat3("controllaPos", &paramater_.controllaPos_.x, 0.1f);
		ImGui::DragFloat("reverseMaxTime", &paramater_.reverseMaxTime, 0.01f);
		ImGui::DragFloat("expationMaxTime", &paramater_.expationMaxTime, 0.01f);
		ImGui::DragFloat("initScale_", &paramater_.initScale_, 0.01f);
		ImGui::DragFloat("expationScale_", &paramater_.expationScale_, 0.01f);
		ImGui::DragFloat("kWaitTime", &paramater_.kWaitTime, 0.01f);
		ImGui::DragFloat("bottomAWidth", &paramater_.bottomAWidth, 0.01f);
		ImGui::DragFloat("bottomAHeigth", &paramater_.bottomAHeigth, 0.01f);
		ImGui::DragFloat("controllaWidth", &paramater_.controllaWidth, 0.01f);
		ImGui::DragFloat("controllaHeigth", &paramater_.controllaHeigth, 0.01f);
		ImGui::DragFloat("拡縮イージング最大", &paramater_.scalingEaseTMax, 0.01f);
		ImGui::DragFloat("拡縮(でかい)スケール", &paramater_.expatingScale, 0.01f);

		globalParameter_->ParmSaveForImGui(groupName_);
		ParmLoadForImGui();
		ImGuiManager::GetInstance()->UnSetFont();
		ImGui::PopID();
	}

#endif // _DEBUG
}

void ResultFontA::Reset() {
	
	scalerScale_ = paramater_.initScale_;
	isFinished_ = false;

	// サイズ、位置適応
	sprite_->SetSize(Vector2(paramater_.bottomAWidth * scalerScale_, paramater_.bottomAHeigth * scalerScale_));
	/// スプライト
	sprite_->SetPos(Vector3(paramater_.bottomAPos_));

	easeDirection_ = 1.0f;
	waitTime_ = 0.0f;
	expationEaseTime_ = 0.0f;
	reverseEaseTime_ = 0.0f;

	step_ = Step::EXPATION;
}