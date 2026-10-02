#include "PlayerRoot.h"

#include "Game/Input/GameActions.h"
#include "Game/GameObj/Player/Player.h"
#include "Game/GameObj/Player/Behavior/PlayerJump.h"
#include "Game/GameObj/Player/Behavior/PlayerAvoid.h"

using namespace Core;


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
		if (Game::Action::kJump.Press() && !owner_->GetIsFall()) {
			step_ = Step::TOJUMP;
			break;
		}
		if (Game::Action::kAvoid.Press() && !owner_->GetIsFall() && owner_->GetAvoidCoolTime() <= 0.0f) {
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
