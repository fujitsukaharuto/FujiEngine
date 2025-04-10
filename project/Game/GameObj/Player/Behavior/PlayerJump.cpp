#include "PlayerJump.h"

#include "Game/GameObj/Player/Player.h"
#include "Game/GameObj/Player/Behavior/PlayerRoot.h"

PlayerJump::PlayerJump(Player* pPlayer) : BasePlayerBehavior(pPlayer) {

	step_ = Step::JUMP;
	speed_ = pPlayer_->GetJumpSpeed();
	pPlayer_->SetFallSpeed(speed_);
	pPlayer->SetIsFall(true);

}

PlayerJump::~PlayerJump() {
}

void PlayerJump::Update() {
	switch (step_) {
		///---------------------------------------------------------------------------------------
		/// ジャンプ
		///---------------------------------------------------------------------------------------
	case PlayerJump::Step::JUMP:

		//移動、ジャンプ
		pPlayer_->Move(pPlayer_->GetMoveSpeed());
		pPlayer_->Jump(speed_);

		// ジャンプ終了
		if (pPlayer_->GetIsFall()) break;
		step_ = Step::TOROOT;

		break;
		///---------------------------------------------------------------------------------------
		/// 通常に移行
		///---------------------------------------------------------------------------------------
	case PlayerJump::Step::TOROOT:
		pPlayer_->SetIsFall(false);
		pPlayer_->ChangeBehavior(std::make_unique<PlayerRoot>(pPlayer_));
		break;
	default:
		break;
	}
}

void PlayerJump::Debug() {
}
