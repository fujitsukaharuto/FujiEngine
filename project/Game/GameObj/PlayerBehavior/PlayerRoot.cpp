/// behavior
#include"PlayerRoot.h"
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
PlayerRoot::PlayerRoot(Player* boss)
	: BasePlayerBehavior("PlayerRoot", boss) {

	/// ===================================================
	///変数初期化
	/// ===================================================
	

}

PlayerRoot ::~PlayerRoot() {

}

//更新
void PlayerRoot::Update() {

	pPlayer_->Move(pPlayer_->GetMoveSpeed());

	//　ジャンプに切り替え
	if (Input::GetInstance()->PushKey(DIK_J)) {
		pPlayer_->ChangeBehavior(std::make_unique<PlayerJump>(pPlayer_));
	}
	else {
		JumpForJoyState();//コントローラジャンプ
	}
}

void PlayerRoot::JumpForJoyState() {
	if (!(Input::GetInstance()->GetGamepadState(joyState))) return;

	if (!((joyState.Gamepad.wButtons & XINPUT_GAMEPAD_A))) return;

	pPlayer_->ChangeBehavior(std::make_unique<PlayerJump>(pPlayer_));

}


void  PlayerRoot::Debug() {
#ifdef _DEBUG
	ImGui::Text("Root");
#endif // _DEBUG	
}

