#include "PlayerRoot.h"

#include "Engine/Input/Input.h"

#include "Game/GameObj/Player/Player.h"
#include "Game/GameObj/Player/Behavior/PlayerJump.h"

PlayerRoot::PlayerRoot(Player* pPlayer) : BasePlayerBehavior(pPlayer) {
	step_ = Step::ROOT;
}

PlayerRoot::~PlayerRoot() {
}

void PlayerRoot::Update() {

	switch (step_) {
		///---------------------------------------------------------------------------------------
		/// 通常
		///---------------------------------------------------------------------------------------
	case PlayerRoot::Step::ROOT:

		if (Input::GetInstance()->PushKey(DIK_SPACE) && !pPlayer_->GetIsFall()) {
			step_ = Step::TOJUMP;
			break;
		}

		pPlayer_->Move(pPlayer_->GetMoveSpeed());
		pPlayer_->Fall(speed_);
		if (pPlayer_->GetIsMove())pPlayer_->SetIsFall(true);

		break;
		///---------------------------------------------------------------------------------------
		/// ジャンプへ移行
		///---------------------------------------------------------------------------------------
	case PlayerRoot::Step::TOJUMP:
		pPlayer_->ChangeBehavior(std::make_unique<PlayerJump>(pPlayer_));
		break;
	default:
		break;
	}

}

void PlayerRoot::Debug() {
}
