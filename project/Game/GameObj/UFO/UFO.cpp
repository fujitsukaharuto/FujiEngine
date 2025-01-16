#include "UFO.h"
///behavior
#include"Behavior/UFORoot.h"
#include"Behavior/UFOPopEnemy.h"
#include"Behavior/UFODamage.h"
//obj
#include"GameObj/Enemy/EnemyManager.h"
#include"GameObj/Enemy/BaseEnemy.h"

///* imgui
#include<imgui.h> 


float UFO::InitY_ = 30.5f;

UFO::UFO() {}

/// ===================================================
///  初期化
/// ===================================================
void UFO::Initialize() {

	OriginGameObject::Initialize();
	OriginGameObject::SetModel("ufo_test.obj");

	///* グローバルパラメータ
	globalParameter_ = GlobalVariables::GetInstance();
	globalParameter_->CreateGroup(groupName_, false);
	AddParmGroup();
	ApplyGlobalParameter();

	moveRestrectionPos_ = 25.0f;
	moveDirection = 1.0f;
	MaxHp_ = 30.0f;
	hp_ = MaxHp_;
	model_->transform.scale.x = 10.0f;

	// collider
	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetTag("UFO");
	SetCollisionSize(collisionSize_);
	collider_->SetParent(model_.get());
	collider_->InfoUpdate();

	/// 通常モードから
	ChangeBehavior(std::make_unique<UFORoot>(this));
}

/// ===================================================
///  更新処理
/// ===================================================
void UFO::Update() {
	popPos_ = model_->GetWorldPos();
	/// ダメージエフェクト
	DamageRendition();
	/// 振る舞い処理
	behavior_->Update();
	///
	collider_->InfoUpdate();
}

/// ===================================================
///  描画
/// ===================================================
void UFO::Draw(Material* material) {

	OriginGameObject::Draw(material);
	collider_->DrawCollider();
}


void UFO::EnemySpawn() {
	pEnemyManager_->SpawnEnemy("NormalEnemy", popPos_);
}

/// ===================================================
///  ダメージ演出
/// ===================================================
void UFO::DamageRendition() {
	
}


///=========================================================
///　移動制限
///==========================================================
//void Player::MoveToLimit() {
//
//	// フィールドの中心とスケールを取得
//	Vector3 fieldCenter = { 0.0f, 0.0f, 0.0f }; // フィールド中心 
//	Vector3 fieldScale = Field::baseScale_;     // フィールドのスケール
//
//	// プレイヤーのスケールを考慮した半径
//	float radiusX = fieldScale.x - model_->transform.scale.x;
//	float radiusZ = fieldScale.z - model_->transform.scale.z;
//
//	// 現在位置が範囲内かチェック
//	bool insideX = std::abs(model_->transform.translate.x - fieldCenter.x) <= radiusX;
//	bool insideZ = std::abs(model_->transform.translate.z - fieldCenter.z) <= radiusZ;
//
//	///-----------------------------------------------------------
//	///範囲外なら戻す
//	///-----------------------------------------------------------
//
//	if (!insideX) {/// X座標
//		model_->transform.translate.x = std::clamp(
//			model_->transform.translate.x,
//			fieldCenter.x - radiusX,
//			fieldCenter.x + radiusX
//		);
//	}
//
//	if (!insideZ) {/// Z座標
//		model_->transform.translate.z = std::clamp(
//			model_->transform.translate.z,
//			fieldCenter.z - radiusZ,
//			fieldCenter.z + radiusZ
//		);
//	}
//
//	// 範囲外の反発処理
//	if (!insideX || !insideZ) {
//		Vector3 directionToCenter = (fieldCenter - model_->transform.translate).Normalize();
//		model_->transform.translate.x += directionToCenter.x * 0.1f; // 軽く押し戻す
//		model_->transform.translate.z += directionToCenter.z * 0.1f; // 軽く押し戻す
//	}
//}

///==========================================================
/// ダメージ受ける
///==========================================================
void UFO::TakeDamageForPar(const float& par) {

	//割合によるインクる面とする値を決める
	float decrementSize = MaxHp_ * par;
	// HP減少
	hp_ -= decrementSize;

	//HPが0以下にならないように
	if (hp_ <= 0) {
		// 死亡処理
		isDeath_ = true;
	}
}

void UFO::Move() {
	model_->transform.translate.x += 3.0f * moveDirection * FPSKeeper::NormalDeltaTime();
	if (std::abs(model_->transform.translate.x) > moveRestrectionPos_) {
		moveDirection = -moveDirection;
		model_->transform.translate.x = Clamp(model_->transform.translate.x, -moveRestrectionPos_, moveRestrectionPos_);
	}
}

///=========================================================
/// 振る舞い切り替え
///==========================================================
void UFO::ChangeBehavior(std::unique_ptr<BaseUFOBehavior>behavior) {
	//引数で受け取った状態を次の状態としてセット
	behavior_ = std::move(behavior);
}
///=========================================================
/// パラメータ調整
///==========================================================
void UFO::AdjustParm() {
	SetValues();
#ifdef _DEBUG
	if (ImGui::CollapsingHeader(groupName_.c_str())) {
		ImGui::PushID(groupName_.c_str());
		/// 位置
		ImGui::SeparatorText("Transform");
		ImGui::DragFloat3("Position", &model_->transform.translate.x, 0.1f);
		ImGui::DragFloat3("CollisionSize", &collisionSize_.x, 0.1f);

		///　Floatのパラメータ
		ImGui::SeparatorText("FloatParamater");
		ImGui::DragFloat("PopWaitTime(s)", &popWaitTime_, 0.01f);
		ImGui::DragFloat("DamageTime(s)", &damageTime_, 0.01f);
		ImGui::DragFloat("DamageDistance", &dagameDistance_, 0.01f);
		ImGui::DragFloat("DamageValue(par)", &damageValue_, 0.01f);
			
		/// セーブとロード
		globalParameter_->ParmSaveForImGui(groupName_);
		ParmLoadForImGui();
		ImGui::PopID();
	}

#endif // _DEBUG
}
///=================================================================================
/// ロード
///=================================================================================
void UFO::ParmLoadForImGui() {

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
void UFO::AddParmGroup() {

	globalParameter_->AddItem(groupName_, "Translate", model_->transform.translate);
	globalParameter_->AddItem(groupName_, "PopWaitTime", popWaitTime_);
	globalParameter_->AddItem(groupName_, "DamageTime", damageTime_);
	globalParameter_->AddItem(groupName_, "DamageDistance", dagameDistance_);
	globalParameter_->AddItem(groupName_, "DamageValue(par)",damageValue_);
	globalParameter_->AddItem(groupName_, "collisionSize_", collisionSize_);
}

///=================================================================================
///パラメータをグループに追加
///=================================================================================
void UFO::SetValues() {
	
	globalParameter_->SetValue(groupName_, "Translate", model_->transform.translate);
	globalParameter_->SetValue(groupName_, "collisionSize_", collisionSize_);
	globalParameter_->SetValue(groupName_, "PopWaitTime", popWaitTime_);
	globalParameter_->SetValue(groupName_, "DamageTime", damageTime_);
	globalParameter_->SetValue(groupName_, "DamageDistance", dagameDistance_);
	globalParameter_->SetValue(groupName_, "DamageValue(par)", damageValue_);
	
}

///=====================================================
///  ImGuiからパラメータを得る
///===================================================== 
void UFO::ApplyGlobalParameter() {
	model_->transform.translate = globalParameter_->GetValue<Vector3>(groupName_, "Translate");
	collisionSize_ = globalParameter_->GetValue<Vector3>(groupName_, "collisionSize_");
	popWaitTime_ = globalParameter_->GetValue<float>(groupName_, "PopWaitTime");
	damageTime_ = globalParameter_->GetValue<float>(groupName_, "DamageTime");
	dagameDistance_ = globalParameter_->GetValue<float>(groupName_, "DamageDistance");
	damageValue_ = globalParameter_->GetValue<float>(groupName_, "DamageValue(par)");
}
///=========================================================
/// Class Set
///==========================================================

void UFO::SetEnemyManager(EnemyManager* enemymanager) {
	pEnemyManager_ = enemymanager;
}

void UFO::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {

	if (other.tag == "EnemyJump") {
		if (dynamic_cast<UFODamage*>(behavior_.get()))return;
		ChangeBehavior(std::make_unique<UFODamage>(this));
	}

}

void UFO::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {

}

void UFO::SetColor(const Vector4& color) {
	model_->SetColor(color);
}

void  UFO::SetCollisionSize(const Vector3& size) {
	collider_->SetWidth(size.x);
	collider_->SetHeight(size.y);
	collider_->SetDepth(size.z);
}