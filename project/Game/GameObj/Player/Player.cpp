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
///state
#include"GameObj/Player/State/PlayerDeath.h"
#include"GameObj/Player/State/PlayerNoneState.h"
//* obj
#include"GameObj/Field/Field.h"
#include"GameObj/FieldBlock/FieldBlockManager.h"
///* std
#include<algorithm>
#include<numbers>
///* imgui
#include"ImGuiManager.h"
#include<imgui.h> 


float Player::InitY_ = 2.2f;

Player::Player() {}

/// ===================================================
///  初期化
/// ===================================================
void Player::Initialize() {

	OriginGameObject::Initialize();
	OriginGameObject::CreateModel("Player.obj");
	tags_[static_cast<size_t>(KikPower::WEAK)] = "WeakKik";
	tags_[static_cast<size_t>(KikPower::MAXPOWER)] = "MaxPowerKik";

	collisionSize_ = Vector3(3, 5, 15);

	///* グローバルパラメータ
	globalParameter_ = GlobalVariables::GetInstance();
	globalParameter_->CreateGroup(groupName_, false);
	AddParmGroup();
	ApplyGlobalParameter();

	kikDirectionView_ = std::make_unique<KikDirectionView>();
	kikDirectionView_->Initialize();

	/// collider
	kikCollider_ = std::make_unique<AABBCollider>();
	kikCollider_->SetTag(tags_[static_cast<size_t>(KikPower::WEAK)]);
	kikCollider_->SetParent(GetModel());
	kikCollider_->SetWidth(collisionSize_.x);
	kikCollider_->SetHeight(collisionSize_.y);
	kikCollider_->SetDepth(collisionSize_.z);
	kikCollider_->SetIsCollisonCheck(false);
	kikCollider_->SetPos(Vector3(0, -1.0f, 0.5f));
	kikCollider_->InfoUpdate();

	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
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

	//パラメータセット
	deathCount_ = paramater_.deathCount_;

	// パーツモデル
	for (int i = 0; i < partsModel_.size(); i++) {
		partsModel_[i] = std::make_unique<Object3d>();
		partsModel_[i]->Create("PlayerFoot.obj");
		partsModel_[i]->SetParent(model_.get());
	}
	// 位置
	partsModel_[static_cast<size_t>(Parts::LEFT)]->transform.translate = paramater_.footStartPosLeft_;
	partsModel_[static_cast<size_t>(Parts::RIGHT)]->transform.translate = paramater_.footStartPosRight_;

	/// 通常モードから
	ChangeBehavior(std::make_unique<PlayerRoot>(this));
	ChangeAttackBehavior(std::make_unique<PlayerAttackRoot>(this));
	ChangeState(std::make_unique<PlayerNoneState>(this));
}

/// ===================================================
///  更新処理
/// ===================================================
void Player::Update() {
	
	//位置デバッグ
	partsModel_[static_cast<size_t>(Parts::LEFT)]->transform.translate = paramater_.footStartPosLeft_;
	partsModel_[static_cast<size_t>(Parts::RIGHT)]->transform.translate = paramater_.footStartPosRight_;

	/// 振る舞い処理
	if (!dynamic_cast<PlayerDeath*>(state_.get())) {
		behavior_->Update();
		attackBehavior_->Update();
	}
	state_->Update();

	//　移動制限
	MoveToLimit();
	ChangeKikDirection();// キック向き

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
	partsModel_[static_cast<size_t>(Parts::LEFT)]->Draw(material);
	partsModel_[static_cast<size_t>(Parts::RIGHT)]->Draw(material);
	
	if (dynamic_cast<PlayerKikAttack*>(attackBehavior_.get())) {
		kikModel_->Draw();
	}
	kikDirectionView_->Draw(material);
#ifdef _DEBUG
	kikCollider_->DrawCollider();
	collider_->DrawCollider();
#endif // _DEBUG
}



/// ===================================================
///  移動入力処理
/// ===================================================
Vector3 Player::GetInputVelocity() {
	inputDirection_ = { 0.0f, 0.0f, 0.0f };
	Input* input = Input::GetInstance();

	if (input->PushKey(DIK_A) || input->PushKey(DIK_D)) {
		if (input->PushKey(DIK_A)) {
			inputDirection_.x -= 1.0f;  // 左移動
		}
		else if (input->PushKey(DIK_D)) {
			inputDirection_.x += 1.0f;  // 右移動
		}
	}
	// ジョイスティック入力
	else if (input->GetGamepadState(joyState)) {
		// DPAD
		if (joyState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT||
			joyState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) {

			if (joyState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) {
				inputDirection_.x -= 1.0f;
			}
			else if (joyState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) {
				inputDirection_.x += 1.0f;
			}
		}
		else {
			///　Stick
			inputDirection_.x += (float)joyState.Gamepad.sThumbLX / SHRT_MAX;
			/*	velocity.z += (float)joyState.Gamepad.sThumbLY / SHRT_MAX;*/
		}
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
		if (!dynamic_cast<PlayerJump*>(behavior_.get())) {
			MoveMotion(paramater_.moveSpeed_);
		}

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
		else if (input->PushKey(DIK_D)) {
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

		// DPAD
		if (joyState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT ||
			joyState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) {

			if (joyState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) {
				StickVelocity.x -= 1.0f;
			}
			else if (joyState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) {
				StickVelocity.x += 1.0f;
			}
		}
		/// Stick
		else {
			
			StickVelocity = { (float)joyState.Gamepad.sThumbLX / SHRT_MAX, 0, 0 };
		}

		//移動条件
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
	model_->transform.translate.y += speed * FPSKeeper::DeltaTimeRate();
	Fall(speed, true);

}

///=========================================================
///　落ちる
///==========================================================
void Player::Fall(float& speed, const bool& isJump) {
	if (!isJump) {
		// 移動
		model_->transform.translate.y += speed * FPSKeeper::DeltaTimeRate();
	}
	// 加速する
	speed = max(speed - (paramater_.gravity_ * FPSKeeper::DeltaTimeRate()), -paramater_.maxFallSpeed_);

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
	Vector3 fieldCenter = Field::baseTranslate_; // フィールド中心 
	Vector3 fieldScale = Field::baseScale_;     // フィールドのスケール

	// プレイヤーのスケールを考慮した半径
	float radiusX = fieldScale.x - model_->transform.scale.x;

	// 現在位置が範囲内かチェック
	bool insideX = std::abs(model_->transform.translate.x - fieldCenter.x) <= radiusX;

	///-----------------------------------------------------------
	/// 範囲外なら処理を実行
	///-----------------------------------------------------------
	if (!insideX) {
		// 範囲外の場合の位置補正
		model_->transform.translate.x = std::clamp(
			model_->transform.translate.x,
			fieldCenter.x - radiusX,
			fieldCenter.x + radiusX
		);

		// 範囲外の反発処理
		Vector3 directionToCenter = (fieldCenter - model_->transform.translate).Normalize();
		model_->transform.translate.x += directionToCenter.x * 0.1f; // 軽く押し戻す
	}
}


///=========================================================
/// ダメージ受ける
///==========================================================
void Player::TakeDamage() {

}


/// ===================================================
///  ダメージ演出
/// ===================================================
void Player::DamageRendition(const float& interval) {
	// 時間の更新
	elapsedTime_ += FPSKeeper::DeltaTimeRate();

	// 一定時間ごとに透明状態を切り替え
	if (elapsedTime_ >= interval) {
		isTransparent_ = !isTransparent_; // 透明状態をトグル

		// 透明状態に応じて色を変更
		if (isTransparent_) {
			model_->SetColor(Vector4(1, 1, 1, 0)); // 透明
		}
		else {
			model_->SetColor(Vector4(1, 1, 1, 1)); // 不透明
		}

		elapsedTime_ = 0.0f; // 経過時間をリセット
	}
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

void Player::ChangeState(std::unique_ptr<BasePlayerState>behavior) {
	state_ = std::move(behavior);
}

///=========================================================
/// パラメータ調整
///==========================================================
void Player::AdjustParm() {
	SetValues();
#ifdef _DEBUG
	if (ImGui::CollapsingHeader(groupName_.c_str())) {
		ImGui::PushID(groupName_.c_str());
		ImGuiManager::GetInstance()->SetFontJapanese();/// 日本語
		/// 位置
		ImGui::SeparatorText("位置");
		ImGui::DragFloat3("本体", &model_->transform.translate.x, 0.1f);
		ImGui::DragFloat3("足左", &paramater_.footStartPosLeft_.x, 0.1f);
		ImGui::DragFloat3("足右", &paramater_.footStartPosRight_.x, 0.1f);

		///　Floatのパラメータ
		ImGui::SeparatorText("基本移動");
		ImGui::DragFloat("ジャンプの高さ", &paramater_.jumpSpeed_, 0.01f);
		ImGui::DragFloat("ジャンプ中の移動スピード", &paramater_.airMoveSpeed_, 0.01f);
		ImGui::DragFloat("移動スピード", &paramater_.moveSpeed_, 0.01f);
		ImGui::DragFloat("重力", &paramater_.gravity_, 0.01f);
		ImGui::DragFloat("最大重力", &paramater_.maxFallSpeed_, 0.01f);
		ImGui::SeparatorText("キック");
		ImGui::DragFloat("キックの当たり判定適応時間(秒)", &paramater_.kikTime_, 0.01f);
		ImGui::DragFloat("キックの回転にかかる時間(秒)", &paramater_.kikRotateTime_, 0.01f);
		ImGui::DragFloat("キックのクールタイム(秒)", &paramater_.kikWaitTime_, 0.01f);
		ImGui::DragFloat("キックのチャージタイム(秒)", &paramater_.kikChargeTime_, 0.01f);
		ImGui::DragFloat("キックの向き変更スピード", &paramater_.kikDirectionSpeed_, 0.01f);
		ImGui::SeparatorText("死亡、リスポーン");
		ImGui::SliderInt("何回当たったら死ぬか(初期化のみ適応)", &paramater_.deathCount_, 1, 3);
		ImGui::DragFloat3("復活座標", &paramater_.respownPos_.x, 0.1f);
		ImGui::DragFloat("復活までの待機時間(秒)", &paramater_.respownWaitTime_, 0.01f);
		ImGui::DragFloat("復活後の無敵時間(秒)", &paramater_.respownInvincibleTime_, 0.01f);
		ImGui::SeparatorText("足の動きモーション");
		ImGui::DragFloat("足の移動量", &paramater_.footMotionAmount_, 0.01f);
		ImGui::DragFloat("足の挙動スピード(普通移動)", &paramater_.moveFootSpeed_, 0.01f);
		ImGui::DragFloat("足の挙動スピード(ジャンプ)", &paramater_.jumpFootSpeed_, 0.01f);
		/*ImGui::SeparatorText("いらないかもパラメータ");
		ImGui::DragFloat("specialAttackAntiTime_", &paramater_.specialAttackAntiTime_, 0.01f);
		ImGui::DragFloat("specialAttackFallSpeed_", &paramater_.specialAttackFallSpeed_, 0.01f);
		ImGui::DragFloat("specialAttackPostJump_", &paramater_.specialAttackPostJump_, 0.01f);
		ImGui::DragFloat("specialAttackPostGravity_", &paramater_.specialAttackPostGravity_, 0.01f);
		ImGui::DragFloat("specialAttackPostMaxFallSpeed_", &paramater_.specialAttackPostMaxFallSpeed_, 0.01f);
		ImGui::DragFloat("RecoilSpeed", &paramater_.recoilSpeed_, 0.01f);
		ImGui::DragFloat("キック当てた時の上に飛ぶ高さ", &paramater_.recoilJumpSpeed_, 0.01f);*/


		ImGuiManager::GetInstance()->UnSetFont();
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
	globalParameter_->AddItem(groupName_, "kikDirectionSpeed_", paramater_.kikDirectionSpeed_);
	globalParameter_->AddItem(groupName_, "kikChargeTime_", paramater_.kikChargeTime_);
	globalParameter_->AddItem(groupName_, "deathCount_", paramater_.deathCount_);
	globalParameter_->AddItem(groupName_, "respownWaitTime_", paramater_.respownWaitTime_);
	globalParameter_->AddItem(groupName_, "respownInvincibleTime_", paramater_.respownInvincibleTime_);
	globalParameter_->AddItem(groupName_, "respownPos_", paramater_.respownPos_);
	globalParameter_->AddItem(groupName_, "kikRotateTime_", paramater_.kikRotateTime_);
	globalParameter_->AddItem(groupName_, "footStartPosLeft_", paramater_.footStartPosLeft_);
	globalParameter_->AddItem(groupName_, "footStartPosRight_", paramater_.footStartPosRight_);
	globalParameter_->AddItem(groupName_, "footMotionAmount_", paramater_.footMotionAmount_);
	globalParameter_->AddItem(groupName_, "moveFootSpeed_", paramater_.moveFootSpeed_);
	globalParameter_->AddItem(groupName_, "jumpFootSpeed_", paramater_.jumpFootSpeed_);
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
	globalParameter_->SetValue(groupName_, "kikDirectionSpeed_", paramater_.kikDirectionSpeed_);
	globalParameter_->SetValue(groupName_, "kikChargeTime_", paramater_.kikChargeTime_);
	globalParameter_->SetValue(groupName_, "deathCount_", paramater_.deathCount_);
	globalParameter_->SetValue(groupName_, "respownWaitTime_", paramater_.respownWaitTime_);
	globalParameter_->SetValue(groupName_, "respownInvincibleTime_", paramater_.respownInvincibleTime_);
	globalParameter_->SetValue(groupName_, "respownPos_", paramater_.respownPos_);
	globalParameter_->SetValue(groupName_, "kikRotateTime_", paramater_.kikRotateTime_);
	globalParameter_->SetValue(groupName_, "footStartPosLeft_", paramater_.footStartPosLeft_);
	globalParameter_->SetValue(groupName_, "footStartPosRight_", paramater_.footStartPosRight_);
	globalParameter_->SetValue(groupName_, "footMotionAmount_", paramater_.footMotionAmount_);
	globalParameter_->SetValue(groupName_, "moveFootSpeed_", paramater_.moveFootSpeed_);
	globalParameter_->SetValue(groupName_, "jumpFootSpeed_", paramater_.jumpFootSpeed_);
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
	paramater_.kikDirectionSpeed_ = globalParameter_->GetValue<float>(groupName_, "kikDirectionSpeed_");
	paramater_.kikChargeTime_ = globalParameter_->GetValue<float>(groupName_, "kikChargeTime_");
	paramater_.deathCount_ = globalParameter_->GetValue<int>(groupName_, "deathCount_");
	paramater_.respownWaitTime_ = globalParameter_->GetValue<float>(groupName_, "respownWaitTime_");
	paramater_.respownInvincibleTime_ = globalParameter_->GetValue<float>(groupName_, "respownInvincibleTime_");
	paramater_.respownPos_ = globalParameter_->GetValue<Vector3>(groupName_, "respownPos_");
	paramater_.kikRotateTime_ = globalParameter_->GetValue<float>(groupName_, "kikRotateTime_");
	paramater_.footStartPosLeft_ = globalParameter_->GetValue<Vector3>(groupName_, "footStartPosLeft_");
	paramater_.footStartPosRight_ = globalParameter_->GetValue<Vector3>(groupName_, "footStartPosRight_");
	paramater_.footMotionAmount_ = globalParameter_->GetValue<float>(groupName_, "footMotionAmount_");
	paramater_.moveFootSpeed_ = globalParameter_->GetValue<float>(groupName_, "moveFootSpeed_");
	paramater_.jumpFootSpeed_ = globalParameter_->GetValue<float>(groupName_, "jumpFootSpeed_");
}
///=========================================================
/// Class Set
///==========================================================

void Player::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {

	// 死にます
	if (other.tag == "DaungerousFieldBlock") {
		deathCount_--; // デクリメント

		/// 死亡
		if (deathCount_ <= 0) {
			ChangeState(std::make_unique<PlayerDeath>(this));
			return;
		}
		/// ダメージ演出
		else {
			

			return;
		}
	}
}

void Player::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {

	/*if (other.tag == "SpecialAttackArea") {
		if (!dynamic_cast<PlayerSpecialFall*>(attackBehavior_.get())) {
			ChangeAttackBehavior(std::make_unique<PlayerSpecialFall>(this));
			return;
		}
	}*/

	
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


void Player::ChangeKikDirection() {
	Input* input = Input::GetInstance();


	float moveSpeed = paramater_.kikDirectionSpeed_ * FPSKeeper::DeltaTimeRate(); // 速度をdeltaTimeで調整

	// 入力に応じて X軸方向の変更量を決定
	float xMove = 0.0f;

	///----------------------------------------------------
	///  keyBorad
	///----------------------------------------------------
	if (input->PushKey(DIK_LEFT)) {
		xMove = -moveSpeed;
	}
	if (input->PushKey(DIK_RIGHT)) {
		xMove = moveSpeed;
	}

	///----------------------------------------------------
	///  JoyStick
	///----------------------------------------------------
	if (Input::GetInstance()->GetGamepadState(joyState)) {
		if ((joyState.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)) {
			xMove = -moveSpeed;
		}
		else if ((joyState.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)) {
			xMove = moveSpeed;
		}
	}

	// X軸方向の変更を適用
	kikDirection_.x += xMove;
	// 正規化処理
	kikDirection_.Normalize();

	// 傾き制限と、常に上向きにする処理
	float maxY = 0.9f; // 上向きを維持する為のY軸の最低値 
	float minX = -0.9f; // X軸の傾きの最小値
	float maxX = 0.9f; // X軸の傾きの最大値

	// X軸の制限
	kikDirection_.x = std::clamp(kikDirection_.x, minX, maxX);
	// 緩やかに上向きを維持
	kikDirection_.y = max(kikDirection_.y, maxY);	
	// 再度正規化
	kikDirection_.Normalize();
	// Z成分を0に設定
	kikDirection_.z = 0.0f;

	// 可視化
	kikDirectionView_->SetDirection(kikDirection_);
	kikDirectionView_->SetStartPos(Vector3(kikCollider_->GetPos().x, kikCollider_->GetPos().y, 2.0f));
	kikDirectionView_->Update();
}

void Player::SetTag(const int& i) {
	kikCollider_->SetTag(tags_[i]);
}

void Player::SetDamageRenditionReset() {
	elapsedTime_ = 0.0f;
	isTransparent_ = false;
}


void Player::MoveMotion(const float &moveSpeed) {
	if (moveSpeed != 0.0f) {
		// モーションの経過時間を更新
		motionTime_ += moveSpeed * FPSKeeper::DeltaTimeRate();

		// 各足のZ軸方向の動きを計算
		float leftFootZOffset = paramater_.footMotionAmount_ * std::sin(motionTime_ + 0.0f);
		float rightFootZOffset = paramater_.footMotionAmount_ * std::sin(motionTime_ + std::numbers::pi_v<float>);

		// 足のモデルに動きを適用
		partsModel_[static_cast<size_t>(Parts::LEFT)]->transform.translate.z = paramater_.footStartPosLeft_.z + leftFootZOffset;
		partsModel_[static_cast<size_t>(Parts::RIGHT)]->transform.translate.z = paramater_.footStartPosRight_.z + rightFootZOffset;
	}
}