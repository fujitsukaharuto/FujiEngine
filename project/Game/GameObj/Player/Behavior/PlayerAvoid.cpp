#include "PlayerAvoid.h"

#include "Game/GameObj/Player/Player.h"
#include "Game/GameObj/Player/Behavior/PlayerRoot.h"

PlayerAvoid::PlayerAvoid(Player* pPlayer) : BasePlayerBehavior(pPlayer) {

	step_ = Step::AVOID;
	avoidTime_ = 0.0f;
	pPlayer_->SetIsNowAvoid(true);

}

PlayerAvoid::~PlayerAvoid() {
}

void PlayerAvoid::Update() {
	switch (step_) {
		///---------------------------------------------------------------------------------------
		/// 回避
		///---------------------------------------------------------------------------------------
	case PlayerAvoid::Step::AVOID:

		pPlayer_->Avoid(avoidTime_);

		if (avoidTime_ >= 30.0f)
		step_ = Step::TOROOT;

		break;
		///---------------------------------------------------------------------------------------
		/// 通常に移行
		///---------------------------------------------------------------------------------------
	case PlayerAvoid::Step::TOROOT:
		pPlayer_->SetIsNowAvoid(false);
		pPlayer_->ChangeBehavior(std::make_unique<PlayerRoot>(pPlayer_));
		break;
	default:
		break;
	}
}

void PlayerAvoid::Debug() {
}
