#include "FieldBlock.h"
#include"GameObj/Player/Player.h"
///*behavior
#include"State/FieldBlockNormal.h"
#include"State/FieldBlockDaungerous.h"
#include"Behavior/FieldBlockNoDamage.h"
#include"Behavior/FieldBlockTakeDamage.h"

#include"Field/Field.h"
///* std
#include<algorithm>
#include"ImGuiManager.h"
#include<imgui.h>

///==========================================================
///　static 変数初期化
///==========================================================
uint32_t FieldBlock::nextSerialNum_ = 0;

FieldBlock::FieldBlock() {
	// シリアル番号を振る
	serialNum_ = nextSerialNum_;
	// 次の番号を加算
	++nextSerialNum_;
}

///========================================================
///  初期化
///========================================================
void FieldBlock::Initialize() {

	modelNames_[static_cast<size_t>(Tag::NORMAL)] = "NormalGround.obj";
	modelNames_[static_cast<size_t>(Tag::DAUNGEROUS)] = "DangerGround.obj";

	/// テクスチャ
	damageStates_[static_cast<size_t>(TEXTURE::DAUNGEROUST)] = "DangerGround_color.png";
	damageStates_[static_cast<size_t>(TEXTURE::DAMAGETWO)] = "HibiGround_color2.png";
	damageStates_[static_cast<size_t>(TEXTURE::DAMAGEONE)] = "HibiGround_color1.png";
	damageStates_[static_cast<size_t>(TEXTURE::NODAMAGE)] = "NormalGround_color.png";

	tags_[static_cast<size_t>(Tag::NORMAL)] = "NormalFieldBlock";
	tags_[static_cast<size_t>(Tag::DAUNGEROUS)] = "DaungerousFieldBlock";

	// 基底クラスの初期化
	OriginGameObject::Initialize();
	OriginGameObject::CreateModel(modelNames_[static_cast<size_t>(Tag::NORMAL)]);

	groupName_ = "FieldBlock" + std::to_string(serialNum_);

	///* グローバルパラメータ
	globalParameter_ = GlobalVariables::GetInstance();
	globalParameter_->CreateGroup(groupName_, false);
	AddParmGroup();
	ApplyGlobalParameter();

	// collider
	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetTag(tags_[static_cast<size_t>(Tag::NORMAL)]);
	SetCollisionSize(colliderSize_);
	collider_->SetParent(model_.get());
	collider_->InfoUpdate();

	// 初期パラメータセット
	Reset();
}

void FieldBlock::Reset() {
	// 初期パラメータセット
	ChangeState(std::make_unique<FieldBlockNormal>(this));
	ChangeBehavior(std::make_unique<FieldBlockNoDamage>(this)); 
}

///========================================================
/// 更新
///========================================================
void FieldBlock::Update() {

	// 振る舞い更新
	state_->Update();
	behavior_->Update();
	SetCollisionSize(colliderSize_);
	// collider更新
	collider_->InfoUpdate();
}

///========================================================
/// 描画
///========================================================
void FieldBlock::Draw(Material* material) {
	OriginGameObject::Draw(material);
#ifdef _DEBUG
	/*collider_->DrawCollider();*/
#endif // _DEBUG
}


void FieldBlock::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {

	///---------------------------------------------------------------------------------------
	/// 落ちてる爆弾との諸突
	///---------------------------------------------------------------------------------------
	if (other.tag == "FallEnemy") {
		if (dynamic_cast<FieldBlockDaungerous*>(state_.get()))return;
		if (dynamic_cast<FieldBlockTakeDamage*>(behavior_.get()))return;
		ChangeBehavior(std::make_unique<FieldBlockTakeDamage>(this));
	}
}


void  FieldBlock::OnCollisionStay(const ColliderInfo& other) {
	if (other.tag == "Player") {
		
	}
}

void FieldBlock::SetParm(const Paramater& paramater) {
	paramater_ = paramater;

}


void FieldBlock::SetPlayer(Player* player) {
	pPlayer_ = player;
}


void FieldBlock::ChangeState(std::unique_ptr<BaseFieldBlockState>behavior) {
	//引数で受け取った状態を次の状態としてセット
	state_ = std::move(behavior);
}

void FieldBlock::ChangeBehavior(std::unique_ptr<BaseFieldBlockBehavior>behavior){
	behavior_ = std::move(behavior);
}

///=================================================================================
/// ロード
///=================================================================================
void FieldBlock::ParmLoadForImGui() {

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
void FieldBlock::AddParmGroup() {

	// Position
	//globalParameter_->AddSeparatorText("Position");
	globalParameter_->AddItem(groupName_, "Translate", model_->transform.translate);
	globalParameter_->AddItem(groupName_, "collisionSize_", colliderSize_);
	globalParameter_->AddItem(groupName_, "LooksSize", model_->transform.scale);
}


///=================================================================================
///パラメータをグループに追加
///=================================================================================
void FieldBlock::SetValues() {

	// Position
	//globalParameter_->AddSeparatorText("Position");
	globalParameter_->SetValue(groupName_, "Translate", model_->transform.translate);
	globalParameter_->SetValue(groupName_, "collisionSize_", colliderSize_);
	globalParameter_->SetValue(groupName_, "LooksSize", model_->transform.scale);
}


///=====================================================
///  ImGuiからパラメータを得る
///===================================================== 
void FieldBlock::ApplyGlobalParameter() {
	// Position
	model_->transform.translate = globalParameter_->GetValue<Vector3>(groupName_, "Translate");
	colliderSize_ = globalParameter_->GetValue<Vector3>(groupName_, "collisionSize_");
	model_->transform.scale = globalParameter_->GetValue<Vector3>(groupName_, "LooksSize");
}

///=========================================================
/// パラメータ調整
///==========================================================
void FieldBlock::AdjustParm() {
	SetValues();
#ifdef _DEBUG

	if (ImGui::CollapsingHeader(groupName_.c_str())) {
		ImGui::PushID(groupName_.c_str());
		ImGuiManager::GetInstance()->SetFontJapanese();

		ImGui::DragFloat3("座標", &model_->transform.translate.x, 0.1f);
		ImGui::DragFloat3("見た目スケール", &model_->transform.scale.x, 0.01f);
		ImGui::SeparatorText("プログラマーが操作する箇所");
		ImGui::DragFloat3("当たり判定サイズ", &colliderSize_.x, 0.01f);

		/// セーブとロード
		globalParameter_->ParmSaveForImGui(groupName_);
		ParmLoadForImGui();
		ImGuiManager::GetInstance()->UnSetFont();
		ImGui::PopID();
	}

#endif // _DEBUG
}

void FieldBlock::SetCollisionSize(const Vector3& size) {
	collider_->SetWidth(size.x);
	collider_->SetHeight(size.y);
	collider_->SetDepth(size.z);
}

void FieldBlock::SetTag(const int& i) {
	collider_->SetTag(tags_[i]);
}

void FieldBlock::ChangeModel(const int& i) {
	OriginGameObject::SetModel(modelNames_[i]);
}

void FieldBlock::ChangeTexture(const int& i) {
	
	model_->SetTexture(damageStates_[i]);
}
