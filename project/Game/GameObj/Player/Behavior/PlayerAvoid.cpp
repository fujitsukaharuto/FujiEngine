#include "PlayerAvoid.h"

#include "Game/GameObj/Player/Player.h"
#include "Game/GameObj/Player/Behavior/PlayerRoot.h"

PlayerAvoid::PlayerAvoid(Player* pPlayer) : BasePlayerBehavior(pPlayer) {

	step_ = Step::AVOID;
	avoidTime_ = 0.0f;
	owner_->SetIsNowAvoid(true);

}

PlayerAvoid::~PlayerAvoid() {
}

void PlayerAvoid::Update() {
	switch (step_) {
		///---------------------------------------------------------------------------------------
		/// 回避
		///---------------------------------------------------------------------------------------
	case PlayerAvoid::Step::AVOID:

		owner_->Avoid(avoidTime_);

		if (avoidTime_ >= 30.0f)
		step_ = Step::TOROOT;

		break;
		///---------------------------------------------------------------------------------------
		/// 通常に移行
		///---------------------------------------------------------------------------------------
	case PlayerAvoid::Step::TOROOT:
		owner_->SetIsNowAvoid(false);
		owner_->ChangeBehavior(std::make_unique<PlayerRoot>(owner_));
		break;
	default:
		break;
	}
}
