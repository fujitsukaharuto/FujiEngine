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


	charge1_->SetParent(&pPlayer_->GetModel()->transform);
	charge2_->SetParent(&pPlayer_->GetModel()->transform);
	charge3_->SetParent(&pPlayer_->GetModel()->transform);
	chargeLight_->SetParent(&pPlayer_->GetModel()->transform);
	chargeRay_->SetParent(&pPlayer_->GetModel()->transform);
	chargeWave_->SetParent(&pPlayer_->GetModel()->transform);
	chargeCircle_->SetParent(&pPlayer_->GetModel()->transform);

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

		if (Input::GetInstance()->PushKey(DIK_J) && coolTime_ <= 0.0f) {
			pPlayer_->InitBullet();
			chargeTime_ = 0.0f;
			coolTime_ = 30.0f;
			step_ = Step::CHAREGE;
			break;
		}
		if (coolTime_ > 0.0f) {
			coolTime_ -= FPSKeeper::DeltaTime();
		}


		break;
		///---------------------------------------------------------------------------------------
		/// ジャンプへ移行
		///---------------------------------------------------------------------------------------
	case PlayerAttackRoot::Step::CHAREGE:
		
		if (Input::GetInstance()->PushKey(DIK_J)) {
			chargeTime_ += FPSKeeper::DeltaTime();
		}
		if (!Input::GetInstance()->PushKey(DIK_J) || pPlayer_->GetIsStrongState()) {
			step_ = Step::ROOT;
			if (pPlayer_->GetIsStrongState()) {
				pPlayer_->StrngthBullet();
			}
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
