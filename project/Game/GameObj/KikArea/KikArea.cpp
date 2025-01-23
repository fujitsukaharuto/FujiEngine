#include "KikArea.h"

#include "assert.h"
#include<imgui.h>


KikArea::KikArea() {}

KikArea::~KikArea() {}

void KikArea::Initialize() {

	OriginGameObject::Initialize();

	///* グローバルパラメータ
	globalParameter_ = GlobalVariables::GetInstance();
	globalParameter_->CreateGroup(groupName_, false);
	AddParmGroup();
	ApplyGlobalParameter();

	/// collider
	weakArea_ = std::make_unique<AABBCollider>();
	weakArea_->SetTag("WeakArea");
	weakArea_->InfoUpdate();

	normalArea_ = std::make_unique<AABBCollider>();
	normalArea_->SetTag("NormalArea");

	normalArea_->InfoUpdate();

	maxPowerArea_ = std::make_unique<AABBCollider>();
	maxPowerArea_->SetTag("MaxPowerArea");
	maxPowerArea_->SetPos(maxPowerAreaPos_);
	maxPowerArea_->InfoUpdate();

	specialAttackArea_ = std::make_unique<AABBCollider>();
	specialAttackArea_->SetTag("SpecialAttackArea");
	specialAttackArea_->SetPos(specialAttackPos_);
	specialAttackArea_->InfoUpdate();

	//サイズセット
	SetAreaSize(weakArea_.get(), weakAreaScale_);
	SetAreaSize(normalArea_.get(), normalAreaScale_);
	SetAreaSize(maxPowerArea_.get(), maxPowerAreaScale_);
	SetAreaSize(specialAttackArea_.get(), specialAttackScale_);
}

void KikArea::Update() {
	OriginGameObject::Update();

	weakArea_->SetPos(weakAreaPos_);
	normalArea_->SetPos(normalAreaPos_);
	maxPowerArea_->SetPos(maxPowerAreaPos_);
	specialAttackArea_->SetPos(specialAttackPos_);

	SetAreaSize(weakArea_.get(), weakAreaScale_);
	SetAreaSize(normalArea_.get(), normalAreaScale_);
	SetAreaSize(maxPowerArea_.get(), maxPowerAreaScale_);
	SetAreaSize(specialAttackArea_.get(), specialAttackScale_);

	weakArea_->InfoUpdate();
	normalArea_->InfoUpdate();
	maxPowerArea_->InfoUpdate();
	specialAttackArea_->InfoUpdate();

}


void KikArea::Draw(Material* material) {
	material;
	/*weakArea_->DrawCollider();
	normalArea_->DrawCollider();
	maxPowerArea_->DrawCollider();*/
	specialAttackArea_->DrawCollider();
}


///=================================================================================
/// ロード
///=================================================================================
void KikArea::ParmLoadForImGui() {

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
void KikArea::AddParmGroup() {

	// Position
	//globalParameter_->AddSeparatorText("Position");
	globalParameter_->AddItem(groupName_, "weakAreaPos_", weakAreaPos_);
	globalParameter_->AddItem(groupName_, "weakAreaScale_", weakAreaScale_);

	globalParameter_->AddItem(groupName_, "normalAreaPos_", normalAreaPos_);
	globalParameter_->AddItem(groupName_, "normalAreaScale_", normalAreaScale_);

	globalParameter_->AddItem(groupName_, "maxPowerAreaPos_", maxPowerAreaPos_);
	globalParameter_->AddItem(groupName_, "maxPowerAreaScale_", maxPowerAreaScale_);

	globalParameter_->AddItem(groupName_, "specialAttackPos_", specialAttackPos_);
	globalParameter_->AddItem(groupName_, "specialAttackScale_", specialAttackScale_);

}


///=================================================================================
///パラメータをグループに追加
///=================================================================================
void KikArea::SetValues() {


	globalParameter_->SetValue(groupName_, "weakAreaPos_", weakAreaPos_);
	globalParameter_->SetValue(groupName_, "weakAreaScale_", weakAreaScale_);

	globalParameter_->SetValue(groupName_, "normalAreaPos_", normalAreaPos_);
	globalParameter_->SetValue(groupName_, "normalAreaScale_", normalAreaScale_);

	globalParameter_->SetValue(groupName_, "maxPowerAreaPos_", maxPowerAreaPos_);
	globalParameter_->SetValue(groupName_, "maxPowerAreaScale_", maxPowerAreaScale_);

	globalParameter_->SetValue(groupName_, "specialAttackPos_", specialAttackPos_);
	globalParameter_->SetValue(groupName_, "specialAttackScale_", specialAttackScale_);

}


///=====================================================
///  ImGuiからパラメータを得る
///===================================================== 
void KikArea::ApplyGlobalParameter() {
	// Position
	weakAreaPos_ = globalParameter_->GetValue<Vector3>(groupName_, "weakAreaPos_");
	normalAreaPos_ = globalParameter_->GetValue<Vector3>(groupName_, "normalAreaPos_");
	maxPowerAreaPos_ = globalParameter_->GetValue<Vector3>(groupName_, "maxPowerAreaPos_");
	specialAttackPos_ = globalParameter_->GetValue<Vector3>(groupName_, "specialAttackPos_");


	weakAreaScale_ = globalParameter_->GetValue<Vector3>(groupName_, "weakAreaScale_");
	normalAreaScale_ = globalParameter_->GetValue<Vector3>(groupName_, "normalAreaScale_");
	maxPowerAreaScale_ = globalParameter_->GetValue<Vector3>(groupName_, "maxPowerAreaScale_");
	specialAttackScale_ = globalParameter_->GetValue<Vector3>(groupName_, "specialAttackScale_");

}

///=========================================================
/// パラメータ調整
///==========================================================
void KikArea::AdjustParm() {
	SetValues();
#ifdef _DEBUG

	if (ImGui::CollapsingHeader(groupName_.c_str())) {
		ImGui::PushID(groupName_.c_str());

		ImGui::SeparatorText("WeakArea");
		ImGui::PushID("WeakArea");
		ImGui::DragFloat3("Pos", &weakAreaPos_.x, 0.1f);
		ImGui::DragFloat3("Scale", &weakAreaScale_.x, 0.1f);
		ImGui::PopID();

		ImGui::SeparatorText("NormalArea");
		ImGui::PushID("NormalArea");
		ImGui::DragFloat3("Pos", &normalAreaPos_.x, 0.1f);
		ImGui::DragFloat3("Scale", &normalAreaScale_.x, 0.1f);
		ImGui::PopID();

		ImGui::SeparatorText("MaxPowerArea");
		ImGui::PushID("MaxPowerArea");
		ImGui::DragFloat3("Pos", &maxPowerAreaPos_.x, 0.1f);
		ImGui::DragFloat3("Scale", &maxPowerAreaScale_.x, 0.1f);
		ImGui::PopID();

		ImGui::SeparatorText("SpecialAttackArea");
		ImGui::PushID("SpecialAttackArea");
		ImGui::DragFloat3("Pos", &specialAttackPos_.x, 0.1f);
		ImGui::DragFloat3("Scale", &specialAttackScale_.x, 0.1f);
		ImGui::PopID();

		/// セーブとロード
		globalParameter_->ParmSaveForImGui(groupName_);
		ParmLoadForImGui();
		ImGui::PopID();
	}

#endif // _DEBUG
}

void KikArea::SetAreaSize(AABBCollider* collider, const Vector3& size) {
	collider->SetWidth(size.x);
	collider->SetHeight(size.y);
	collider->SetDepth(size.z);
}