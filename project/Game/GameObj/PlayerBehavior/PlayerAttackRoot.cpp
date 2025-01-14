/// behavior
#include"PlayerAttackRoot.h"
#include"PlayerJump.h"
#include"PlayerKikAttack.h"

/// boss
#include"GameObj/Player/Player.h"
/// frame
#include"DX/FPSKeeper.h"
/// inupt
#include"Input/Input.h"
#include"GameObj/JoyState/JoyState.h"

#include<imgui.h>

//初期化
PlayerAttackRoot::PlayerAttackRoot(Player* boss)
	: BasePlayerAttackBehavior("PlayerAttackRoot", boss) {

	/// ===================================================
	/// 変数初期化
	/// ===================================================
	
	
}

PlayerAttackRoot ::~PlayerAttackRoot() {

}

//更新
void PlayerAttackRoot::Update() {
	switch (step_)
	{
	case PlayerAttackRoot::Step::WAIT:
		waitTime_ += FPSKeeper::NormalDeltaTime();
		if (waitTime_ < pPlayer_->GetKikWaitTime())break;
		step_ = Step::ATTACK;
		break;
	case PlayerAttackRoot::Step::ATTACK:
		//　キックする
		if (Input::GetInstance()->PushKey(DIK_K)) {
			pPlayer_->ChangeAttackBehavior(std::make_unique<PlayerKikAttack>(pPlayer_));
		}
		else {
			KikAttackState();//キック

		}
		break;
	default:
		break;
	}
	
	
}

void PlayerAttackRoot::KikAttackState() {
	if (!(Input::GetInstance()->GetGamepadState(joyState))) return;

	if (!((joyState.Gamepad.wButtons & XINPUT_GAMEPAD_B))) return;

	pPlayer_->ChangeAttackBehavior(std::make_unique<PlayerKikAttack>(pPlayer_));
}


void  PlayerAttackRoot::Debug() {

}

