#include "PlayerJump.h"

#include "Game/GameObj/Player/Player.h"
#include "Game/GameObj/Player/Behavior/PlayerRoot.h"
#include "Engine/Core/Input/Input.h"

using namespace Core;


PlayerJump::PlayerJump(Player* pPlayer) : BasePlayerBehavior(pPlayer) {

	step_ = Step::JUMP;
	speed_ = owner_->GetJumpSpeed();
	owner_->SetFallSpeed(speed_);
	pPlayer->SetIsFall(true);

}

PlayerJump::~PlayerJump() {
}

void PlayerJump::Update() {

	Input* input = Input::GetInstance();
	switch (step_) {
		///---------------------------------------------------------------------------------------
		/// ジャンプ
		///---------------------------------------------------------------------------------------
	case PlayerJump::Step::JUMP:

		if ((input->TriggerKey(DIK_SPACE) || input->TriggerButton(PadInput::X)) && isSecondJump_) {
			if (speed_ > 0.0f) {
				speed_ += owner_->GetSecondJumpSpeed();
			} else {
				speed_ = owner_->GetSecondJumpSpeed();
			}
			speed_ = owner_->GetJumpSpeed();
			owner_->SetFallSpeed(speed_);
			owner_->SetIsFall(true);
			isSecondJump_ = false;
		}

		//移動、ジャンプ
		owner_->Move(owner_->GetMoveSpeed());
		owner_->Jump(speed_);

		// ジャンプ終了
		if (owner_->GetIsFall()) break;
		step_ = Step::TOROOT;

		break;
		///---------------------------------------------------------------------------------------
		/// 通常に移行
		///---------------------------------------------------------------------------------------
	case PlayerJump::Step::TOROOT:
		owner_->SetIsFall(false);
		owner_->ChangeBehavior(std::make_unique<PlayerRoot>(owner_));
		break;
	default:
		break;
	}
}
