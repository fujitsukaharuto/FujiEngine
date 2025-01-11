/// behavior
#include"PlayerKikAttack.h"
#include"PlayerJump.h"

/// boss
#include"GameObj/Player/Player.h"
/// frame
#include"DX/FPSKeeper.h"
/// inupt
#include"Input/Input.h"
#include"GameObj/JoyState/JoyState.h"

#include<imgui.h>

//初期化
PlayerKikAttack::PlayerKikAttack(Player* boss)
	: BasePlayerBehavior("PlayerKikAttack", boss) {

	/// ===================================================
	/// 変数初期化
	/// ===================================================
	

}

PlayerKikAttack ::~PlayerKikAttack() {

}

//更新
void PlayerKikAttack::Update() {

	pPlayer_->Move(pPlayer_->GetMoveSpeed());

	//　ジャンプに切り替え
	if (Input::GetInstance()->PushKey(DIK_J)) {
		pPlayer_->ChangeBehavior(std::make_unique<PlayerJump>(pPlayer_));
	}
	else {
		JumpForJoyState();//コントローラジャンプ
	}
}

void PlayerKikAttack::JumpForJoyState() {
	if (!(Input::GetInstance()->GetGamepadState(joyState))) return;

	if (!((joyState.Gamepad.wButtons & XINPUT_GAMEPAD_A))) return;

	pPlayer_->ChangeBehavior(std::make_unique<PlayerJump>(pPlayer_));

}


void  PlayerKikAttack::Debug() {
#ifdef _DEBUG
	ImGui::Text("Root");
#endif // _DEBUG	
}

