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

/////* obj
#include"Field/Field.h"

///* std
#include<algorithm>

///* imgui
#ifdef _DEBUG
#include"imgui.h"
#endif 

///プレイヤーの仕様書**************************************************************************
/// https://docs.google.com/document/u/0/d/1lUpgCc6XqLgLKEJcuu56m6yqqhdGDwCGz_9dRb5ebM0/edit
///*****************************************************************************************

float Player::InitY_ = 1.0f;

Player::Player() {}


/// ===================================================
///  初期化
/// ===================================================
void Player::Initialize() {

	jumpSpeed_ = 0.0f;
	muzzelJumpSpeed_ = 1.5f;

	OriginGameObject::Initialize();
	OriginGameObject::SetModel("player.obj");

	/// 通常モードから
	ChangeBehavior(std::make_unique<PlayerRoot>(this));
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
	//　移動制限
	MoveToLimit();
	// 落ちる
	Fall();

	/// 更新
	//base::Update();
}


/// ===================================================
///  落ちる
/// ===================================================
void Player::Fall() {
	if (!dynamic_cast<PlayerJump*>(behavior_.get())) {
	
		// 移動
		
		model_->transform.translate.y += fallSpeed_;
		// 重力加速度
		const float kGravityAcceleration = 3.4f * FPSKeeper::DeltaTime();
		// 加速度ベクトル
		float accelerationY = -kGravityAcceleration;
		// 加速する
		fallSpeed_ = max(fallSpeed_ + accelerationY, -0.75f);

		// 着地
		if (model_->transform.translate.y <= Player::InitY_) {
			model_->transform.translate.y = Player::InitY_;
		}
	}
}



/// ===================================================
///  描画
/// ===================================================
void Player::Draw(Material* material) {

	OriginGameObject::Draw(material);
}


/// ===================================================
///  ダメージ演出
/// ===================================================
void Player::DamageRendition() {
	if (isDamage_) {
		damageTime_ -= FPSKeeper::DeltaTime();

		// ダメージ時間がまだ残っている場合

		blinkTimer_ += FPSKeeper::DeltaTime();

		// チカチカ間隔ごとに透明フラグを切り替え
		if (blinkTimer_ >= blinkInterval_) {
			blinkTimer_ = 0.0f;          // チカチカタイマーをリセット
			isTransparent_ = !isTransparent_; // 透明フラグを反転

			//if (isTransparent_) {
			//	objColor_.SetColor(Vector4(1.0f, 1.0f, 1.0f, 0.0f)); // 透明色
			//}
			//else {
			//	objColor_.SetColor(Vector4(0.5f, 0.0f, 0.0f, 0.99f)); // 赤常色
			//}
		}

		if (damageTime_ <= 0.0f) {
			// ダメージ時間が終了したらフラグをリセット
			isDamage_ = false;
			isTransparent_ = false;
			//objColor_.SetColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f)); // 通常色に戻す
		}
	}
}


/// ===================================================
///  移動入力処理
/// ===================================================
Vector3 Player::GetInputVelocity() {
	Vector3 velocity = { 0.0f, 0.0f, 0.0f };
	Input* input = Input::GetInstance();

	// キーボード入力
	if (input->PushKey(DIK_W)) {
		velocity.z += 1.0f;  // 前進
	}
	if (input->PushKey(DIK_S)) {
		velocity.z -= 1.0f;  // 後退
	}
	if (input->PushKey(DIK_A)) {
		velocity.x -= 1.0f;  // 左移動
	}
	if (input->PushKey(DIK_D)) {
		velocity.x += 1.0f;  // 右移動
	}

	// ジョイスティック入力
	if (input->GetGamepadState(joyState)) {
		velocity.x += (float)joyState.Gamepad.sThumbLX / SHRT_MAX;
		velocity.z += (float)joyState.Gamepad.sThumbLY / SHRT_MAX;
	}

	return velocity;
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
		velocity_ = (velocity_).Normalize() * speed;

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
	///  JoyStick
	///----------------------------------------------------
	
	if (Input::GetInstance()->GetGamepadState(joyState)) {
		// 移動量
		StickVelocity = { (float)joyState.Gamepad.sThumbLX / SHRT_MAX, 0, (float)joyState.Gamepad.sThumbLY / SHRT_MAX };
		if ((StickVelocity).Length() > thresholdValue) {
			isMoving = true;
		}
	}

	///----------------------------------------------------
	///  keyBorad
	///----------------------------------------------------
	else {
		// キーボード入力
		if (input->PushKey(DIK_W)) {
			keyBoradVelocity.z += 1.0f;  // 前進
		}
		if (input->PushKey(DIK_S)) {
			keyBoradVelocity.z -= 1.0f;  // 後退
		}
		if (input->PushKey(DIK_A)) {
			keyBoradVelocity.x -= 1.0f;  // 左移動
		}
		if (input->PushKey(DIK_D)) {
			keyBoradVelocity.x += 1.0f;  // 右移動
		}
		if ((keyBoradVelocity).Length() > 0) {
			isMoving = true;
		}
	}
	return isMoving;
}

/// ===================================================
///  Player Jump
/// ===================================================

void Player::Jump(float& speed) {
	// 移動
	model_->transform.translate.y += speed;
	// 重力加速度
	const float kGravityAcceleration = 4.4f * FPSKeeper::DeltaTime();
	// 加速度ベクトル
	float accelerationY = -kGravityAcceleration;
	// 加速する
	speed = max(speed + accelerationY, jumpLimit_);

	// 着地
	if (model_->transform.translate.y <= Player::InitY_) {
		model_->transform.translate.y = Player::InitY_;

		// ジャンプ終了
		ChangeBehavior(std::make_unique<PlayerRoot>(this));
	}
}

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

	//////////////////////////////////////////////////////
	///範囲外なら戻す
	//////////////////////////////////////////////////////

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
	if (!isDamage_) {
		life_--;
		isDamage_ = true;
		damageTime_ = damageCollTime_;
	}
}

///=========================================================
/// 振る舞い切り替え
///==========================================================
void Player::ChangeBehavior(std::unique_ptr<BasePlayerBehavior>behavior) {
	//引数で受け取った状態を次の状態としてセット
	behavior_ = std::move(behavior);
}

void Player::Debug() {

}

///=========================================================
/// Class Set
///==========================================================

