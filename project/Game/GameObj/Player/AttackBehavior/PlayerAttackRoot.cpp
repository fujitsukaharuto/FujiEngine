#include "PlayerAttackRoot.h"

#include "Engine/Input/Input.h"

#include "Game/GameObj/Player/Player.h"

PlayerAttackRoot::PlayerAttackRoot(Player* pPlayer) : BasePlayerAttackBehavior(pPlayer) {
	step_ = Step::ROOT;
	chargeTime_ = 0.0f;
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
		if (chargeTime_ >= pPlayer_->GetMaxChargeTime()) {
			step_ = Step::STRONGSHOT;
		}

		break;
	case PlayerAttackRoot::Step::STRONGSHOT:
		
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
