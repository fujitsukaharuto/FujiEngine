#include "PlayerAttackRoot.h"

#include "Engine/Input/Input.h"

#include "Game/GameObj/Player/Player.h"

PlayerAttackRoot::PlayerAttackRoot(Player* pPlayer) : BasePlayerAttackBehavior(pPlayer) {
	step_ = Step::ROOT;
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
			step_ = Step::CHAREGE;
			break;
		}



		break;
		///---------------------------------------------------------------------------------------
		/// ジャンプへ移行
		///---------------------------------------------------------------------------------------
	case PlayerAttackRoot::Step::CHAREGE:
		
		break;
	case PlayerAttackRoot::Step::STRONGSHOT:
		
		break;
	default:
		break;
	}

}

void PlayerAttackRoot::Debug() {
}
