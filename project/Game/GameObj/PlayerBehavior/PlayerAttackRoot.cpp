/// behavior
#include"PlayerAttackRoot.h"
#include"PlayerJump.h"
#include"PlayerKikAttack.h"

/// boss
#include"GameObj/Player/Player.h"
/// frame
#include"DX/FPSKeeper.h"
/// inupt
#include"Input/Input.h"
#include"GameObj/JoyState/JoyState.h"
#include "Particle/ParticleManager.h"

#include<imgui.h>

//初期化
PlayerAttackRoot::PlayerAttackRoot(Player* boss)
	: BasePlayerAttackBehavior("PlayerAttackRoot", boss) {

	/// ===================================================
	/// 変数初期化
	/// ===================================================
	chargeTime_ = 0.0f;
	pPlayer_->SetTag(static_cast<size_t>(Player::KikPower::WEAK));

	ParticleManager::Load(charge1_, "charge1");
	ParticleManager::Load(charge2_, "charge2");
	ParticleManager::Load(kiran1_, "kiran1");
	ParticleManager::Load(kiran2_, "kiran2");

}

PlayerAttackRoot ::~PlayerAttackRoot() {

}

//更新
void PlayerAttackRoot::Update() {
	switch (step_)
	{
	case PlayerAttackRoot::Step::WAIT:
		/// --------------------------------------------------------------
        ///  キック待機
        /// ---------------------------------------------------------------
		waitTime_ += FPSKeeper::DeltaTimeRate();
		if (waitTime_ < pPlayer_->GetParamater().kikWaitTime_)break;
	
		///* ボタン推したらチャージスタート

		// キー
		 if (Input::GetInstance()->PushKey(DIK_K)) {
			 step_ = Step::ATTACKCHAREGE;
			 break;
		}
		 // コントローラ
		 if (!(Input::GetInstance()->GetGamepadState(joyState)))break;
		 if (!(Input::GetInstance()->PressButton(PadInput::B)))break;
		step_ = Step::ATTACKCHAREGE;

		break;
	case PlayerAttackRoot::Step::ATTACKCHAREGE:
		/// --------------------------------------------------------------
		///  攻撃チャージ
		/// ---------------------------------------------------------------
		
		// チャージ
		if (Input::GetInstance()->PushKey(DIK_K)) {
			chargeTime_ += FPSKeeper::NormalDeltaTime();
			/// チャージパーティクル入れる
			charge1_.pos = pPlayer_->GetModel()->GetWorldPos();
			charge2_.pos = pPlayer_->GetModel()->GetWorldPos();
			charge1_.Emit();

		}
		else {
		
			ChargeForJoyStick();//キック
		}
		
		///* ボタン離したらキック攻撃開始
		if (Input::GetInstance()->ReleaseKey(DIK_K)) {
			pPlayer_->ChangeAttackBehavior(std::make_unique<PlayerKikAttack>(pPlayer_));
			return;
		}
		else {
			AtttackForJoyStick();
		}

		 	///* 時間経過で強化キックに
		if (chargeTime_ < pPlayer_->GetParamater().kikChargeTime_)break;
		pPlayer_->SetTag(static_cast<size_t>(Player::KikPower::MAXPOWER));
		step_ = Step::STRONGATTACK;
		break;
	case PlayerAttackRoot::Step::STRONGATTACK:
		/// --------------------------------------------------------------
		///  強い攻撃
		/// ---------------------------------------------------------------
		

		/// チャージパーティクル(つおい(赤))入れる
		kiran1_.pos = pPlayer_->GetModel()->GetWorldPos();
		kiran2_.pos = pPlayer_->GetModel()->GetWorldPos();
		kiran1_.Emit();

		if (Input::GetInstance()->ReleaseKey(DIK_K)) {
			pPlayer_->ChangeAttackBehavior(std::make_unique<PlayerKikAttack>(pPlayer_));
			return;
		}
		else {
			AtttackForJoyStick();
		}
		break;
	default:
		break;
	}
	
	
}

void PlayerAttackRoot::ChargeForJoyStick() {
	if (!(Input::GetInstance()->GetGamepadState(joyState)))return;

	if (!(Input::GetInstance()->PressButton(PadInput::B)))return;
	chargeTime_ += FPSKeeper::NormalDeltaTime();
	/// チャージパーティクル入れる
}

// ボタン離した時
void PlayerAttackRoot::AtttackForJoyStick() {
	if (!(Input::GetInstance()->GetGamepadState(joyState)))return;

	if (!(Input::GetInstance()->ReleaseButton(PadInput::B)))return;

	pPlayer_->ChangeAttackBehavior(std::make_unique<PlayerKikAttack>(pPlayer_));
}


void  PlayerAttackRoot::Debug() {

}

