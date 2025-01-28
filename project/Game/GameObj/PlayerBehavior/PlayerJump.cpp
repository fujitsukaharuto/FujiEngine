
///* behavior
#include"PlayerJump.h"
#include"PlayerRoot.h"

///* obj
#include"Game/GameObj/Player/Player.h"

///* frame
#include"DX/FPSKeeper.h"
#include "Particle/ParticleManager.h"


/// ===================================================
///初期化
/// ===================================================
PlayerJump::PlayerJump(Player* player)
	: BasePlayerBehavior("PlayerJump", player) {

	///---------------------------------------------------
	///変数初期化
	///---------------------------------------------------

	speed_ = pPlayer_->GetParamater().jumpSpeed_;
	pPlayer_->SetMotionTime(0.0f);

	jumpSE_ = Audio::GetInstance()->SoundLoadWave("jump.wav");
	Audio::GetInstance()->SoundPlayWave(jumpSE_);

	ParticleManager::Load(jumpEmit_, "jump");
	jumpEmit_.pos = pPlayer_->GetModel()->GetWorldPos();
	jumpEmit_.Burst();

	/*pPlayer_->SetJumpSpeed(1.5f);*/
}

PlayerJump ::~PlayerJump() {

}

//更新
void PlayerJump::Update() {
	pPlayer_->MoveMotion(pPlayer_->GetParamater().jumpFootSpeed_);
	pPlayer_->Move(pPlayer_->GetParamater().airMoveSpeed_);
	pPlayer_->Jump(speed_);
}


void  PlayerJump::Debug() {

}