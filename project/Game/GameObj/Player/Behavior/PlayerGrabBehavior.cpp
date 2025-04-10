#include "PlayerGrabBehavior.h"

#include "Engine/Input/Input.h"

#include "Game/Objects3D/Player/Player.h"
#include "Game/Objects3D/Player/Behavior/PlayerRoot.h"

PlayerGrabBehavior::PlayerGrabBehavior(Player* pPlayer) : BasePlayerBehavior(pPlayer) {
	step_ = Step::GRAB;
}

PlayerGrabBehavior::~PlayerGrabBehavior() {
}

void PlayerGrabBehavior::Update() {

	switch (step_) {
	case PlayerGrabBehavior::Step::GRAB:

		/*if (pPlayer_->SearchGrabBlock(pPlayer_->GetGrabDir())) {
			pPlayer_->GrabBlock();
			step_ = Step::GRABMOVE;
			break;
		}*/
		step_ = Step::TOROOT;

		break;
	case PlayerGrabBehavior::Step::GRABMOVE:

		pPlayer_->Move(pPlayer_->GetMoveSpeed());
		pPlayer_->Fall(speed_);
		if (pPlayer_->GetIsMove())pPlayer_->SetIsFall(true);

		if (Input::GetInstance()->PushKey(DIK_J)) {
			break;
		}
		/*pPlayer_->ReleaseGrabBlock();*/
		step_ = Step::TOROOT;

		break;
	case PlayerGrabBehavior::Step::TOROOT:
		pPlayer_->ChangeBehavior(std::make_unique<PlayerRoot>(pPlayer_));
		break;
	default:
		break;
	}

}

void PlayerGrabBehavior::Debug() {
}
