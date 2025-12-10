#include "PlayerAttackRoot.h"
#include "Engine/Audio/AudioPlayer.h"
#include "Engine/Particle/ParticleManager.h"
#include "Engine/Input/Input.h"

#include "Game/GameObj/Player/Player.h"

using namespace Audio;
using namespace Core;
using namespace Graphics;


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

	chargeSE_ = &AudioPlayer::GetInstance()->SoundLoadWave("chargeSE.wav");
	chargeCompleteSE_ = &AudioPlayer::GetInstance()->SoundLoadWave("chargeCompleteSE.wav");
}

PlayerAttackRoot::~PlayerAttackRoot() {
	// いちおう音を安全のため止めておく
	AudioPlayer::GetInstance()->SoundStopWave(*chargeSE_);
	AudioPlayer::GetInstance()->SoundStopWave(*chargeCompleteSE_);
}

void PlayerAttackRoot::Update() {

	Input* input = Input::GetInstance();
	switch (step_) {
		///---------------------------------------------------------------------------------------
		/// 通常
		///---------------------------------------------------------------------------------------
	case PlayerAttackRoot::Step::ROOT:

		if ((input->PushKey(DIK_J) || input->PressButton(PadInput::A)) && coolTime_ <= 0.0f) {
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
		/// チャージ状態
		///---------------------------------------------------------------------------------------
	case PlayerAttackRoot::Step::CHAREGE:
		
		if ((input->PushKey(DIK_J) || input->PressButton(PadInput::A))) {
			chargeTime_ += FPSKeeper::DeltaTime();
		}
		if (!(input->PushKey(DIK_J) || input->PressButton(PadInput::A)) || pPlayer_->GetIsStrongState()) {
			step_ = Step::ROOT;
			if (pPlayer_->GetIsStrongState()) {
				pPlayer_->StrngthBullet();
			}
			pPlayer_->ReleaseBullet();
			AudioPlayer::GetInstance()->SoundStopWave(*chargeSE_);
			isCharegSEStart_ = false;
			chargeTime_ = 0.0f;
		}
		if (chargeTime_ > 10.0f) {
			if (!isCharegSEStart_) {
				AudioPlayer::GetInstance()->SoundLoop(*chargeSE_, 0.04f);
				isCharegSEStart_ = true;
			}
			charge1_->Emit();
			charge2_->Emit();
			charge3_->Emit();
			chargeLight_->Emit();
		}
		if (chargeTime_ >= pPlayer_->GetMaxChargeTime()) {
			step_ = Step::STRONGSHOT;
			chargeRay_->Emit();
			chargeWave_->Emit();
			AudioPlayer::GetInstance()->SoundPlayWave(*chargeCompleteSE_,0.4f);
			pPlayer_->StrngthBullet();
		}

		break;
		///---------------------------------------------------------------------------------------
		/// 強化弾状態
		///---------------------------------------------------------------------------------------
	case PlayerAttackRoot::Step::STRONGSHOT:

		charge1_->Emit();
		charge2_->Emit();
		charge3_->Emit();
		chargeLight_->Emit();
		chargeCircle_->Emit();

		if (!(input->PushKey(DIK_J) || input->PressButton(PadInput::A))) {
			step_ = Step::ROOT;
			AudioPlayer::GetInstance()->SoundStopWave(*chargeSE_);
			pPlayer_->ReleaseBullet();
			isCharegSEStart_ = false;
		}

		break;
	default:
		break;
	}

}

void PlayerAttackRoot::Debug() {
}

void PlayerAttackRoot::ResetParam() {
}

void PlayerAttackRoot::StopSE() {
	AudioPlayer::GetInstance()->SoundStopWave(*chargeSE_);
	AudioPlayer::GetInstance()->SoundStopWave(*chargeCompleteSE_);
}
