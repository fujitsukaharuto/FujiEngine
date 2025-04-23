#include "PlayerAttackRoot.h"

#include "Engine/Particle/ParticleManager.h"
#include "Engine/Input/Input.h"

#include "Game/GameObj/Player/Player.h"

PlayerAttackRoot::PlayerAttackRoot(Player* pPlayer) : BasePlayerAttackBehavior(pPlayer) {
	step_ = Step::ROOT;
	chargeTime_ = 0.0f;

	ParticleManager::LoadParentGroup(charge1_, "ChargeEffect1");
	ParticleManager::LoadParentGroup(charge2_, "ChargeEffect2");
	ParticleManager::LoadParentGroup(charge3_, "ChargeEffect3");
	ParticleManager::LoadParentGroup(chargeLight_, "ChargeLight");
	ParticleManager::LoadParentGroup(chargeRay_, "ChargeRay");
	ParticleManager::LoadParentGroup(chargeWave_, "ChargeWave");
	ParticleManager::LoadParentGroup(chargeCircle_, "ChargeCircle");


	charge1_->SetParent(pPlayer_->GetModel());
	charge2_->SetParent(pPlayer_->GetModel());
	charge3_->SetParent(pPlayer_->GetModel());
	chargeLight_->SetParent(pPlayer_->GetModel());
	chargeRay_->SetParent(pPlayer_->GetModel());
	chargeWave_->SetParent(pPlayer_->GetModel());
	chargeCircle_->SetParent(pPlayer_->GetModel());

	chargeCircle_->grain_.isColorFadeIn_ = true;

	chargeRay_->frequencyTime_ = 0.0f;
	chargeWave_->frequencyTime_ = 0.0f;

}

PlayerAttackRoot::~PlayerAttackRoot() {
}

void PlayerAttackRoot::Update() {

	switch (step_) {
		///---------------------------------------------------------------------------------------
		/// 通常
		///---------------------------------------------------------------------------------------
	case PlayerAttackRoot::Step::ROOT:

		if (Input::GetInstance()->TriggerKey(DIK_J)) {
			pPlayer_->InitBullet();
			chargeTime_ = 0.0f;
			step_ = Step::CHAREGE;
			break;
		}



		break;
		///---------------------------------------------------------------------------------------
		/// ジャンプへ移行
		///---------------------------------------------------------------------------------------
	case PlayerAttackRoot::Step::CHAREGE:
		
		if (Input::GetInstance()->PushKey(DIK_J)) {
			chargeTime_ += FPSKeeper::DeltaTime();
		}
		if (!Input::GetInstance()->PushKey(DIK_J)) {
			step_ = Step::ROOT;
			pPlayer_->ReleaseBullet();
		}
		if (chargeTime_ > 10.0f) {
			charge1_->Emit();
			charge2_->Emit();
			charge3_->Emit();
			chargeLight_->Emit();
		}
		if (chargeTime_ >= pPlayer_->GetMaxChargeTime()) {
			step_ = Step::STRONGSHOT;
			chargeRay_->Emit();
			chargeWave_->Emit();
			pPlayer_->StrngthBullet();
		}

		break;
	case PlayerAttackRoot::Step::STRONGSHOT:

		charge1_->Emit();
		charge2_->Emit();
		charge3_->Emit();
		chargeLight_->Emit();
		chargeCircle_->Emit();

		if (!Input::GetInstance()->PushKey(DIK_J)) {
			step_ = Step::ROOT;
			pPlayer_->ReleaseBullet();
		}

		break;
	default:
		break;
	}

}

void PlayerAttackRoot::Debug() {
}
