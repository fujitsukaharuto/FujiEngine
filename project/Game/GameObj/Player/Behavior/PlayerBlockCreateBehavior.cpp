#include "PlayerBlockCreateBehavior.h"

#include "Engine/Input/Input.h"

#include "Game/Objects3D/Player/Player.h"
#include "Game/Objects3D/Player/Behavior/PlayerRoot.h"

PlayerBlockCreateBehavior::PlayerBlockCreateBehavior(Player* pPlayer) : BasePlayerBehavior(pPlayer) {
	step_ = Step::BLOCKCREATE;
}

PlayerBlockCreateBehavior::~PlayerBlockCreateBehavior() {
}

void PlayerBlockCreateBehavior::Update() {
	switch (step_) {
		///---------------------------------------------------------------------------------------
		/// Createモード
		///---------------------------------------------------------------------------------------
	case PlayerBlockCreateBehavior::Step::BLOCKCREATE:

		if (Input::GetInstance()->TriggerKey(DIK_A) || Input::GetInstance()->TriggerKey(DIK_W) || Input::GetInstance()->TriggerKey(DIK_D)) {
			if (Input::GetInstance()->TriggerKey(DIK_A)) {
				dir_ = -1;
			} else if (Input::GetInstance()->TriggerKey(DIK_D)) {
				dir_ = 1;
			} else if (Input::GetInstance()->TriggerKey(DIK_W)){
				dir_ = 0;
			}
			step_ = Step::BLOCKPOP;
			break;
		}

		if (Input::GetInstance()->PushKey(DIK_J)) break;
		step_ = Step::TOROOT;

		break;
		///---------------------------------------------------------------------------------------
		/// ブロックを設置
		///---------------------------------------------------------------------------------------
	case PlayerBlockCreateBehavior::Step::BLOCKPOP:
		
		/*if (pPlayer_->SearchCreateMapPos(dir_)) {
			pPlayer_->CreateBlock(dir_);
			dir_ = 0;
			step_ = Step::BLOCKPOPING;
		}*/
		dir_ = 0;
		step_ = Step::BLOCKCREATE;

		break;
	case PlayerBlockCreateBehavior::Step::BLOCKPOPING:
		
		step_ = Step::BLOCKCREATE;
		
		break;
		///---------------------------------------------------------------------------------------
		/// 通常に移行
		///---------------------------------------------------------------------------------------
	case PlayerBlockCreateBehavior::Step::TOROOT:
		pPlayer_->ChangeBehavior(std::make_unique<PlayerRoot>(pPlayer_));
		break;
	default:
		break;
	}
}

void PlayerBlockCreateBehavior::Debug() {
}
