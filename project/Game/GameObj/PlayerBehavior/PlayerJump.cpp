
///* behavior
#include"PlayerJump.h"
#include"PlayerRoot.h"

///* obj
#include"Game/GameObj/Player/Player.h"

///* frame
#include"DX/FPSKeeper.h"



/// ===================================================
///初期化
/// ===================================================
PlayerJump::PlayerJump(Player* player)
	: BasePlayerBehavior("PlayerJump", player) {

	///---------------------------------------------------
	///変数初期化
	///---------------------------------------------------

	speed_ = pPlayer_->GetParamater().jumpSpeed_;

	jumpSE_ = Audio::GetInstance()->SoundLoadWave("jump.wav");
	Audio::GetInstance()->SoundPlayWave(jumpSE_);

	/*pPlayer_->SetJumpSpeed(1.5f);*/
}

PlayerJump ::~PlayerJump() {

}

//更新
void PlayerJump::Update() {
	pPlayer_->Move(pPlayer_->GetParamater().airMoveSpeed_);
	pPlayer_->Jump(speed_);
}


void  PlayerJump::Debug() {

}