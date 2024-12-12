
///* behavior
#include"PlayerJump.h"
#include"PlayerRoot.h"

///* obj
#include"Game/GameObj/Player/Player.h"

///* frame
#include"DX/FPSKeeper.h"

///* imgui
#ifdef _DEBUG
#include"imgui.h"
#endif 


/// ===================================================
///初期化
/// ===================================================
PlayerJump::PlayerJump(Player* player)
	: BasePlayerBehavior("PlayerJump", player) {

	///---------------------------------------------------
	///変数初期化
	///---------------------------------------------------

	speed_ = pPlayer_->GetMuzzulJumpSpeed();

	/*pPlayer_->SetJumpSpeed(1.5f);*/
}

PlayerJump ::~PlayerJump() {

}

//更新
void PlayerJump::Update() {
	pPlayer_->Move(0.3f);
	pPlayer_->Jump(speed_);

}


void  PlayerJump::Debug() {
	ImGui::Text("Jump");
}