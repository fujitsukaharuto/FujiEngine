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
#include"GameObj/PlayerBehavior/PlayerRecoil.h"
//* obj
#include"Field/Field.h"
///* std
#include<algorithm>
///* imgui
#include<imgui.h> 

///プレイヤーの仕様書**************************************************************************
/// https://docs.google.com/document/u/0/d/1lUpgCc6XqLgLKEJcuu56m6yqqhdGDwCGz_9dRb5ebM0/edit
///*****************************************************************************************

float Player::InitY_ = 1.5f;

Player::Player() {}

/// ===================================================
///  初期化
/// ===================================================
void Player::Initialize() {

	OriginGameObject::Initialize();
	OriginGameObject::SetModel("player.obj");

	///* グローバルパラメータ
	globalParameter_ = GlobalVariables::GetInstance();
	globalParameter_->CreateGroup(groupName_, false);
	AddParmGroup();
	ApplyGlobalParameter();

	/// collider
	weakikCollider_ = std::make_unique<AABBCollider>();
	weakikCollider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	weakikCollider_->SetTag("WeakKik");
	weakikCollider_->SetParent(GetModel());
	weakikCollider_->SetWidth(5.0f);
	weakikCollider_->SetHeight(5.0f);
	weakikCollider_->SetDepth(20.0f);
	weakikCollider_->SetIsCollisonCheck(false);
	weakikCollider_->SetPos(Vector3(0, 0, 1.5f));
	weakikCollider_->InfoUpdate();

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
	behavior_->Update();
	if (!dynamic_cast<PlayerRecoil*>(behavior_.get())) {
		attackBehavior_->Update();
	}
	//　移動制限
	MoveToLimit();

	weakikCollider_->InfoUpdate();
	
	/// 更新
	//base::Update();
}

/// ===================================================
///  描画
/// ===================================================
void Player::Draw(Material* material) {

	OriginGameObject::Draw(material);
#ifdef _DEBUG
	weakikCollider_->DrawCollider();
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
		velocity_ = (velocity_).Normalize() * (speed*FPSKeeper::NormalDeltaTime());

		// 移動ベクトルをカメラの角度に基づいて回転
		Matrix4x4 rotateMatrix = MakeRotateYMatrix(CameraManager::GetInstance()->GetCamera()->transform.rotate.y);
		velocity_ = TransformNormal(velocity_, rotateMatrix);

		// 位置を更新
		model_->transform.translate += velocity_;

		// 目標角度を計算
		objectiveAngle_ = std::atan2(velocity_.x, velocity_.z);

		// 最短角度補間でプレイヤーの回転を更新
		model_->transform.rotate.y = LerpShortAngle(model_->transform.rotate.y, objectiveAngle_, 0.3f);
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
	Fall(speed,true);
	
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
	speed = max(speed - (gravity_ * FPSKeeper::NormalDeltaTime()), -0.7f);

	// 着地
	if (model_->transform.translate.y  <= Player::InitY_) {
		model_->transform.translate.y  = Player::InitY_;
		speed = 0.0f;

		if (!dynamic_cast<PlayerJump*>(behavior_.get()))return;
			// ジャンプ終了
			ChangeBehavior(std::make_unique<PlayerRoot>(this));	
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
	attackBehavior_= std::move(behavior);
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
		ImGui::DragFloat("jumpSpeed", &jumpSpeed_, 0.01f);
		ImGui::DragFloat("AirMoveSpeed", &airMoveSpeed_, 0.01f);
		ImGui::DragFloat("MoveSpeed", &moveSpeed_, 0.01f);
		ImGui::DragFloat("Gravity", &gravity_, 0.01f);
		ImGui::DragFloat("RecoilSpeed", &recoilSpeed_, 0.01f);
		ImGui::DragFloat("RecoilJumpSpeed", &recoilJumpSpeed_, 0.01f);
		ImGui::DragFloat("KikTime", &kikTime_, 0.01f);
		
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
	globalParameter_->AddItem(groupName_, "JumpSpeed", jumpSpeed_);
	globalParameter_->AddItem(groupName_, "MoveSpeed", moveSpeed_);
	globalParameter_->AddItem(groupName_, "Gravity", gravity_);
	globalParameter_->AddItem(groupName_, "AirMoveSpeed", airMoveSpeed_);
	globalParameter_->AddItem(groupName_, "RecoilSpeed", recoilSpeed_);
	globalParameter_->AddItem(groupName_, "RecoilJumpSpeed", recoilJumpSpeed_);
	globalParameter_->AddItem(groupName_, "KikTime", kikTime_);

}

///=================================================================================
///パラメータをグループに追加
///=================================================================================
void Player::SetValues() {

	globalParameter_->SetValue(groupName_, "Translate", model_->transform.translate);
	globalParameter_->SetValue(groupName_, "JumpSpeed", jumpSpeed_);
	globalParameter_->SetValue(groupName_, "Gravity", gravity_);
	globalParameter_->SetValue(groupName_, "MoveSpeed", moveSpeed_);
	globalParameter_->SetValue(groupName_, "AirMoveSpeed", airMoveSpeed_);
	globalParameter_->SetValue(groupName_, "RecoilSpeed", recoilSpeed_);
	globalParameter_->SetValue(groupName_, "RecoilJumpSpeed", recoilJumpSpeed_);
	globalParameter_->SetValue(groupName_, "KikTime", kikTime_);
}

///=====================================================
///  ImGuiからパラメータを得る
///===================================================== 
void Player::ApplyGlobalParameter() {
	model_->transform.translate = globalParameter_->GetValue<Vector3>(groupName_, "Translate");
	jumpSpeed_ = globalParameter_->GetValue<float>(groupName_, "JumpSpeed");
	gravity_ = globalParameter_->GetValue<float>(groupName_, "Gravity");
	moveSpeed_ = globalParameter_->GetValue<float>(groupName_, "MoveSpeed");
	airMoveSpeed_ = globalParameter_->GetValue<float>(groupName_, "AirMoveSpeed");
	recoilSpeed_ = globalParameter_->GetValue<float>(groupName_, "RecoilSpeed");
	recoilJumpSpeed_ = globalParameter_->GetValue<float>(groupName_, "RecoilJumpSpeed");
	kikTime_ = globalParameter_->GetValue<float>(groupName_, "KikTime");
}
///=========================================================
/// Class Set
///==========================================================

void Player::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {

	if (other.tag == "Enemy") {
		if (dynamic_cast<PlayerRecoil*>(behavior_.get()))return;
		ChangeBehavior(std::make_unique<PlayerRecoil>(this));
		ChangeAttackBehavior(std::make_unique<PlayerAttackRoot>(this));
		return;
	}


}

void Player::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {

}


void  Player::SetIsSetCollision(const bool& is) {
	weakikCollider_->SetIsCollisonCheck(is);
}