#include "Player.h"
///* fps
#include"DX/FPSKeeper.h"
///*input
#include"GameObj/JoyState/JoyState.h"
///* math
#include"Vector2Matrix.h"
///*camera
#include"Camera/CameraManager.h"
///* behavior
#include"GameObj/PlayerBehavior/PlayerRoot.h"
#include"GameObj/PlayerBehavior/PlayerJump.h"
#include"GameObj/PlayerBehavior/PlayerAttackRoot.h"
#include"GameObj/PlayerBehavior/PlayerKikAttack.h"
#include"GameObj/PlayerBehavior/PlayerRecoil.h"
#include"GameObj/PlayerBehavior/PlayerSpecialFall.h"
//* obj
#include"Field/Field.h"
///* std
#include<algorithm>
///* imgui
#include<imgui.h> 


float Player::InitY_ = 1.5f;

Player::Player() {}

/// ===================================================
///  初期化
/// ===================================================
void Player::Initialize() {

	OriginGameObject::Initialize();
	OriginGameObject::SetModel("player.obj");
	tags_[static_cast<size_t>(KikPower::WEAK)] = "WeakKik";
	tags_[static_cast<size_t>(KikPower::NORMAL)] = "NormalKik";
	tags_[static_cast<size_t>(KikPower::MAXPOWER)] = "MaxPowerKik";

	///* グローバルパラメータ
	globalParameter_ = GlobalVariables::GetInstance();
	globalParameter_->CreateGroup(groupName_, false);
	AddParmGroup();
	ApplyGlobalParameter();

	/// collider
	kikCollider_ = std::make_unique<AABBCollider>();
	kikCollider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	kikCollider_->SetTag(tags_[static_cast<size_t>(KikPower::MAXPOWER)]);
	kikCollider_->SetParent(GetModel());
	kikCollider_->SetWidth(2.0f);
	kikCollider_->SetHeight(2.0f);
	kikCollider_->SetDepth(15.0f);
	kikCollider_->SetIsCollisonCheck(false);
	kikCollider_->SetPos(Vector3(0, 0, 1.5f));
	kikCollider_->InfoUpdate();

	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionStayCallback([this](const ColliderInfo& other) {OnCollisionStay(other); });
	collider_->SetTag("Player");
	collider_->SetParent(GetModel());
	collider_->SetWidth(2.0f);
	collider_->SetHeight(2.0f);
	collider_->SetDepth(2.0f);
	collider_->InfoUpdate();

	// 仮モデル
	kikModel_ = std::make_unique<Object3d>();
	kikModel_->Create("NormalEnemy.obj");
	kikModel_->SetParent(GetModel());
	kikModel_->transform.translate.z = 1.5f;

	/// 通常モードから
	ChangeBehavior(std::make_unique<PlayerRoot>(this));
	ChangeAttackBehavior(std::make_unique<PlayerAttackRoot>(this));
}

/// ===================================================
///  更新処理
/// ===================================================
void Player::Update() {
	/*prePos_ = GetWorldPosition();*/
	/// ダメージエフェクト
	DamageRendition();

	/// 振る舞い処理
	if (!dynamic_cast<PlayerSpecialFall*>(attackBehavior_.get())) {
		behavior_->Update();
	}
	attackBehavior_->Update();

	//　移動制限
	MoveToLimit();

	collider_->InfoUpdate();
	kikCollider_->InfoUpdate();

	/// 更新
	//base::Update();
}

/// ===================================================
///  描画
/// ===================================================
void Player::Draw(Material* material) {

	OriginGameObject::Draw(material);
	if (dynamic_cast<PlayerKikAttack*>(attackBehavior_.get())) {
		kikModel_->Draw();
	}
#ifdef _DEBUG
	kikCollider_->DrawCollider();
	collider_->DrawCollider();
#endif // _DEBUG
}


/// ===================================================
///  ダメージ演出
/// ===================================================
void Player::DamageRendition() {

}


/// ===================================================
///  移動入力処理
/// ===================================================
Vector3 Player::GetInputVelocity() {
	inputDirection_ = { 0.0f, 0.0f, 0.0f };
	Input* input = Input::GetInstance();


	if (input->PushKey(DIK_A)) {
		inputDirection_.x -= 1.0f;  // 左移動
	}
	if (input->PushKey(DIK_D)) {
		inputDirection_.x += 1.0f;  // 右移動
	}

	// ジョイスティック入力
	if (input->GetGamepadState(joyState)) {
		inputDirection_.x += (float)joyState.Gamepad.sThumbLX / SHRT_MAX;
		/*	velocity.z += (float)joyState.Gamepad.sThumbLY / SHRT_MAX;*/
	}

	return inputDirection_;
}


/// ===================================================
///  移動処理
/// ===================================================
void Player::Move(const float& speed) {

	/// Inuputから速度代入
	velocity_ = GetInputVelocity();

	/// 移動処理
	if (GetIsMoving()) {
		// 移動ベクトルの正規化と速さの適用
		velocity_ = (velocity_).Normalize() * (speed * FPSKeeper::DeltaTimeRate());

		// 移動ベクトルをカメラの角度に基づいて回転
		Matrix4x4 rotateMatrix = MakeRotateYMatrix(CameraManager::GetInstance()->GetCamera()->transform.rotate.y);
		velocity_ = TransformNormal(velocity_, rotateMatrix);

		// 位置を更新
		model_->transform.translate += velocity_;

		// 目標角度を計算
		objectiveAngle_ = std::atan2(velocity_.x, velocity_.z);

		// 最短角度補間でプレイヤーの回転を更新
		model_->transform.rotate.y = LerpShortAngle(model_->transform.rotate.y, objectiveAngle_, 0.5f);
	}
}

/// ===================================================
///  動いているか
/// ===================================================
bool Player::GetIsMoving() {
	Input* input = Input::GetInstance();
	bool isMoving = false;
	const float thresholdValue = 0.3f;
	Vector3 StickVelocity;
	Vector3 keyBoradVelocity = {};

	///----------------------------------------------------
	///  keyBorad
	///----------------------------------------------------
	if (input->PushKey(DIK_A) || input->PushKey(DIK_D)) {
		if (input->PushKey(DIK_A)) {
			keyBoradVelocity.x -= 1.0f;  // 左移動
		}
		if (input->PushKey(DIK_D)) {
			keyBoradVelocity.x += 1.0f;  // 右移動
		}
		if ((keyBoradVelocity).Length() > 0) {
			isMoving = true;
		};
	}
	///----------------------------------------------------
	///  JoyStick
	///----------------------------------------------------

	else if (Input::GetInstance()->GetGamepadState(joyState)) {
		// 移動量
		StickVelocity = { (float)joyState.Gamepad.sThumbLX / SHRT_MAX, 0, 0 };
		if ((StickVelocity).Length() > thresholdValue) {
			isMoving = true;
		};
	}

	return isMoving;
}

/// ===================================================
///  Player Jump
/// ===================================================

void Player::Jump(float& speed) {
	// 移動
	model_->transform.translate.y += speed;
	Fall(speed, true);

}

///=========================================================
///　落ちる
///==========================================================
void Player::Fall(float& speed, const bool& isJump) {
	if (!isJump) {
		// 移動
		model_->transform.translate.y += speed;
	}
	// 加速する
	speed = max(speed - (paramater_.gravity_ * FPSKeeper::NormalDeltaTime()), -paramater_.maxFallSpeed_);

	// 着地
	if (model_->transform.translate.y <= Player::InitY_) {
		model_->transform.translate.y = Player::InitY_;
		speed = 0.0f;

		if (!dynamic_cast<PlayerJump*>(behavior_.get()))return;
		// ジャンプ終了
		ChangeBehavior(std::make_unique<PlayerRoot>(this));
	}
}

/// ===================================================
///  Player Jump
/// ===================================================

void Player::SpecialPostJump(float& speed) {
	// 移動
	model_->transform.translate.y += speed;
	SpecialPostFall(speed, true);

}

///=========================================================
///　落ちる
///==========================================================
void Player::SpecialPostFall(float& speed, const bool& isJump) {
	if (!isJump) {
		// 移動
		model_->transform.translate.y += speed;
	}
	// 加速する
	speed = max(speed - (paramater_.specialAttackPostGravity_ * FPSKeeper::NormalDeltaTime()), -paramater_.specialAttackPostMaxFallSpeed_);

	// 着地
	if (model_->transform.translate.y <= Player::InitY_) {
		model_->transform.translate.y = Player::InitY_;
		speed = 0.0f;
	}
}

///=========================================================
///　移動制限
///==========================================================
void Player::MoveToLimit() {

	// フィールドの中心とスケールを取得
	Vector3 fieldCenter = { 0.0f, 0.0f, 0.0f }; // フィールド中心 
	Vector3 fieldScale = Field::baseScale_;     // フィールドのスケール

	// プレイヤーのスケールを考慮した半径
	float radiusX = fieldScale.x - model_->transform.scale.x;
	float radiusZ = fieldScale.z - model_->transform.scale.z;

	// 現在位置が範囲内かチェック
	bool insideX = std::abs(model_->transform.translate.x - fieldCenter.x) <= radiusX;
	bool insideZ = std::abs(model_->transform.translate.z - fieldCenter.z) <= radiusZ;

	///-----------------------------------------------------------
	///範囲外なら戻す
	///-----------------------------------------------------------

	if (!insideX) {/// X座標
		model_->transform.translate.x = std::clamp(
			model_->transform.translate.x,
			fieldCenter.x - radiusX,
			fieldCenter.x + radiusX
		);
	}

	if (!insideZ) {/// Z座標
		model_->transform.translate.z = std::clamp(
			model_->transform.translate.z,
			fieldCenter.z - radiusZ,
			fieldCenter.z + radiusZ
		);
	}

	// 範囲外の反発処理
	if (!insideX || !insideZ) {
		Vector3 directionToCenter = (fieldCenter - model_->transform.translate).Normalize();
		model_->transform.translate.x += directionToCenter.x * 0.1f; // 軽く押し戻す
		model_->transform.translate.z += directionToCenter.z * 0.1f; // 軽く押し戻す
	}
}

///=========================================================
/// ダメージ受ける
///==========================================================
void Player::TakeDamage() {

}

///=========================================================
/// 振る舞い切り替え
///==========================================================
void Player::ChangeBehavior(std::unique_ptr<BasePlayerBehavior>behavior) {
	//引数で受け取った状態を次の状態としてセット
	behavior_ = std::move(behavior);
}

void Player::ChangeAttackBehavior(std::unique_ptr<BasePlayerAttackBehavior>behavior) {
	attackBehavior_ = std::move(behavior);
}

///=========================================================
/// パラメータ調整
///==========================================================
void Player::AdjustParm() {
	SetValues();
#ifdef _DEBUG
	if (ImGui::CollapsingHeader(groupName_.c_str())) {
		ImGui::PushID(groupName_.c_str());
		/// 位置
		ImGui::SeparatorText("Transform");
		ImGui::DragFloat3("Position", &model_->transform.translate.x, 0.1f);

		///　Floatのパラメータ
		ImGui::SeparatorText("FloatParamater");
		ImGui::DragFloat("jumpSpeed", &paramater_.jumpSpeed_, 0.01f);
		ImGui::DragFloat("AirMoveSpeed", &paramater_.airMoveSpeed_, 0.01f);
		ImGui::DragFloat("MoveSpeed", &paramater_.moveSpeed_, 0.01f);
		ImGui::DragFloat("Gravity", &paramater_.gravity_, 0.01f);
		ImGui::DragFloat("MaxFallSpeed", &paramater_.maxFallSpeed_, 0.01f);
		ImGui::DragFloat("RecoilSpeed", &paramater_.recoilSpeed_, 0.01f);
		ImGui::DragFloat("RecoilJumpSpeed", &paramater_.recoilJumpSpeed_, 0.01f);
		ImGui::SeparatorText("Kik");
		ImGui::DragFloat("KikTime", &paramater_.kikTime_, 0.01f);
		ImGui::DragFloat("KikWaitTime", &paramater_.kikWaitTime_, 0.01f);
		ImGui::SeparatorText("SpecialAttack");
		ImGui::DragFloat("specialAttackAntiTime_", &paramater_.specialAttackAntiTime_, 0.01f);
		ImGui::DragFloat("specialAttackFallSpeed_", &paramater_.specialAttackFallSpeed_, 0.01f);
		ImGui::DragFloat("specialAttackPostJump_", &paramater_.specialAttackPostJump_, 0.01f);
		ImGui::DragFloat("specialAttackPostGravity_", &paramater_.specialAttackPostGravity_, 0.01f);
		ImGui::DragFloat("specialAttackPostMaxFallSpeed_", &paramater_.specialAttackPostMaxFallSpeed_, 0.01f);


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
void Player::ParmLoadForImGui() {

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
void Player::AddParmGroup() {

	globalParameter_->AddItem(groupName_, "Translate", model_->transform.translate);
	globalParameter_->AddItem(groupName_, "JumpSpeed", paramater_.jumpSpeed_);
	globalParameter_->AddItem(groupName_, "MoveSpeed", paramater_.moveSpeed_);
	globalParameter_->AddItem(groupName_, "Gravity", paramater_.gravity_);
	globalParameter_->AddItem(groupName_, "AirMoveSpeed", paramater_.airMoveSpeed_);
	globalParameter_->AddItem(groupName_, "RecoilSpeed", paramater_.recoilSpeed_);
	globalParameter_->AddItem(groupName_, "RecoilJumpSpeed", paramater_.recoilJumpSpeed_);
	globalParameter_->AddItem(groupName_, "KikTime", paramater_.kikTime_);
	globalParameter_->AddItem(groupName_, "KikWaitTime", paramater_.kikWaitTime_);
	globalParameter_->AddItem(groupName_, "MaxFallSpeed", paramater_.maxFallSpeed_);
	globalParameter_->AddItem(groupName_, "specialAttackAntiTime_", paramater_.specialAttackAntiTime_);
	globalParameter_->AddItem(groupName_, "specialAttackFallSpeed_", paramater_.specialAttackFallSpeed_);
	globalParameter_->AddItem(groupName_, "specialAttackPostJump_", paramater_.specialAttackPostJump_);
	globalParameter_->AddItem(groupName_, "specialAttackPostGravity_", paramater_.specialAttackPostGravity_);
	globalParameter_->AddItem(groupName_, "specialAttackPostMaxFallSpeed_", paramater_.specialAttackPostMaxFallSpeed_);

}

///=================================================================================
///パラメータをグループに追加
///=================================================================================
void Player::SetValues() {

	globalParameter_->SetValue(groupName_, "Translate", model_->transform.translate);
	globalParameter_->SetValue(groupName_, "JumpSpeed", paramater_.jumpSpeed_);
	globalParameter_->SetValue(groupName_, "Gravity", paramater_.gravity_);
	globalParameter_->SetValue(groupName_, "MoveSpeed", paramater_.moveSpeed_);
	globalParameter_->SetValue(groupName_, "AirMoveSpeed", paramater_.airMoveSpeed_);
	globalParameter_->SetValue(groupName_, "RecoilSpeed", paramater_.recoilSpeed_);
	globalParameter_->SetValue(groupName_, "RecoilJumpSpeed", paramater_.recoilJumpSpeed_);
	globalParameter_->SetValue(groupName_, "KikTime", paramater_.kikTime_);
	globalParameter_->SetValue(groupName_, "KikWaitTime", paramater_.kikWaitTime_);
	globalParameter_->SetValue(groupName_, "MaxFallSpeed", paramater_.maxFallSpeed_);
	globalParameter_->SetValue(groupName_, "specialAttackAntiTime_", paramater_.specialAttackAntiTime_);
	globalParameter_->SetValue(groupName_, "specialAttackFallSpeed_", paramater_.specialAttackFallSpeed_);
	globalParameter_->SetValue(groupName_, "specialAttackPostJump_", paramater_.specialAttackPostJump_);
	globalParameter_->SetValue(groupName_, "specialAttackPostGravity_", paramater_.specialAttackPostGravity_);
	globalParameter_->SetValue(groupName_, "specialAttackPostMaxFallSpeed_", paramater_.specialAttackPostMaxFallSpeed_);

}

///=====================================================
///  ImGuiからパラメータを得る
///===================================================== 
void Player::ApplyGlobalParameter() {
	model_->transform.translate = globalParameter_->GetValue<Vector3>(groupName_, "Translate");
	paramater_.jumpSpeed_ = globalParameter_->GetValue<float>(groupName_, "JumpSpeed");
	paramater_.gravity_ = globalParameter_->GetValue<float>(groupName_, "Gravity");
	paramater_.moveSpeed_ = globalParameter_->GetValue<float>(groupName_, "MoveSpeed");
	paramater_.airMoveSpeed_ = globalParameter_->GetValue<float>(groupName_, "AirMoveSpeed");
	paramater_.recoilSpeed_ = globalParameter_->GetValue<float>(groupName_, "RecoilSpeed");
	paramater_.recoilJumpSpeed_ = globalParameter_->GetValue<float>(groupName_, "RecoilJumpSpeed");
	paramater_.kikTime_ = globalParameter_->GetValue<float>(groupName_, "KikTime");
	paramater_.kikWaitTime_ = globalParameter_->GetValue<float>(groupName_, "KikWaitTime");
	paramater_.maxFallSpeed_ = globalParameter_->GetValue<float>(groupName_, "MaxFallSpeed");
	paramater_.specialAttackAntiTime_ = globalParameter_->GetValue<float>(groupName_, "specialAttackAntiTime_");
	paramater_.specialAttackFallSpeed_ = globalParameter_->GetValue<float>(groupName_, "specialAttackFallSpeed_");
	paramater_.specialAttackPostJump_ = globalParameter_->GetValue<float>(groupName_, "specialAttackPostJump_");
	paramater_.specialAttackPostGravity_ = globalParameter_->GetValue<float>(groupName_, "specialAttackPostGravity_");
	paramater_.specialAttackPostMaxFallSpeed_ = globalParameter_->GetValue<float>(groupName_, "specialAttackPostMaxFallSpeed_");
}
///=========================================================
/// Class Set
///==========================================================

void Player::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {

	if (other.tag == "Enemy") {

		SetKikIsCollision(false);
		ChangeBehavior(std::make_unique<PlayerRecoil>(this));
		ChangeAttackBehavior(std::make_unique<PlayerAttackRoot>(this));
		return;
	}
}

void Player::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {

	if (other.tag == "SpecialAttackArea") {
		if (!dynamic_cast<PlayerSpecialFall*>(attackBehavior_.get())) {
			ChangeAttackBehavior(std::make_unique<PlayerSpecialFall>(this));
		}
	}
	else {

		if (other.tag == "WeakArea") {
			kikCollider_->SetTag(tags_[static_cast<size_t>(KikPower::WEAK)]);
		}

		else if (other.tag == "NormalArea") {
			kikCollider_->SetTag(tags_[static_cast<size_t>(KikPower::NORMAL)]);
		}
		else	if (other.tag == "MaxPowerArea") {
			kikCollider_->SetTag(tags_[static_cast<size_t>(KikPower::MAXPOWER)]);
		}
	}
}


void  Player::SetKikIsCollision(const bool& is) {
	kikCollider_->SetIsCollisonCheck(is);
}

/// ===================================================
///  現在の向きを取得（-1: 左, 1: 右）
/// ===================================================
float Player::GetFacingDirection() const {
	// Y軸回転（ラジアン）を取得
	float angle = model_->transform.rotate.y;

	// sin関数を使用して向きを判定（左: 負, 右: 正）
	return (std::sin(angle) >= 0.0f) ? 1.0f : -1.0f;
}
