#include "PlayerRoot.h"

#include "Engine/Input/Input.h"

#include "Game/GameObj/Player/Player.h"
#include "Game/GameObj/Player/Behavior/PlayerJump.h"
#include "Game/GameObj/Player/Behavior/PlayerAvoid.h"

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
		if (Input::GetInstance()->PushKey(DIK_K) && !pPlayer_->GetIsFall() && pPlayer_->GetAvoidCoolTime() <= 0.0f) {
			step_ = Step::TOAVOID;
			break;
		}

		pPlayer_->Move(pPlayer_->GetMoveSpeed());

		break;
		///---------------------------------------------------------------------------------------
		/// ジャンプへ移行
		///---------------------------------------------------------------------------------------
	case PlayerRoot::Step::TOJUMP:
		pPlayer_->ChangeBehavior(std::make_unique<PlayerJump>(pPlayer_));
		break;
	case PlayerRoot::Step::TOAVOID:
		pPlayer_->ChangeBehavior(std::make_unique<PlayerAvoid>(pPlayer_));
		break;
	default:
		break;
	}

}

void PlayerRoot::Debug() {
}
