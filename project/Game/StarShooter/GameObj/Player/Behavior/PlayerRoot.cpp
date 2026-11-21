#include "PlayerRoot.h"

#include "Game/StarShooter/Input/GameActions.h"
#include "Game/StarShooter/GameObj/Player/Player.h"
#include "Game/StarShooter/GameObj/Player/Behavior/PlayerJump.h"
#include "Game/StarShooter/GameObj/Player/Behavior/PlayerAvoid.h"

using namespace Core;
using namespace StarShooter;


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
		if (StarShooter::Action::kJump.Press() && !owner_->GetIsFall()) {
			step_ = Step::TOJUMP;
			break;
		}
		if (StarShooter::Action::kAvoid.Press() && !owner_->GetIsFall() && owner_->GetAvoidCoolTime() <= 0.0f) {
			step_ = Step::TOAVOID;
			break;
		}

		owner_->Move(owner_->GetMoveSpeed());

		break;
		///---------------------------------------------------------------------------------------
		/// ジャンプへ移行
		///---------------------------------------------------------------------------------------
	case PlayerRoot::Step::TOJUMP:
		owner_->ChangeBehavior(std::make_unique<PlayerJump>(owner_));
		break;
		///---------------------------------------------------------------------------------------
		/// 回避へ移行
		///---------------------------------------------------------------------------------------
	case PlayerRoot::Step::TOAVOID:
		owner_->ChangeBehavior(std::make_unique<PlayerAvoid>(owner_));
		break;
	default:
		break;
	}

}
