/// behavior
#include"PlayerDeath.h"
#include"PlayerRespown.h"

/// boss
#include"GameObj/Player/Player.h"
/// frame
#include"DX/FPSKeeper.h"
/// inupt
#include"Input/Input.h"
#include"GameObj/JoyState/JoyState.h"
#include "Particle/ParticleManager.h"

#include<imgui.h>

//初期化
PlayerDeath::PlayerDeath(Player* boss)
	: BasePlayerState("PlayerDeath", boss) {

	/// ===================================================
	/// 変数初期化
	/// ===================================================
	pPlayer_->SetIsDeathUIStart(true);
	step_ = Step::DEATH;

	ParticleManager::Load(deadEmit1_, "playerDead1");
	ParticleManager::Load(deadEmit2_, "playerDead2");
	deadEmit1_.pos = pPlayer_->GetModel()->transform.translate;
	deadEmit2_.pos = pPlayer_->GetModel()->transform.translate;
	deadEmit1_.Burst();
	deadEmit2_.Burst();

	deadSE_ = Audio::GetInstance()->SoundLoadWave("playerDead.wav");
	Audio::GetInstance()->SoundPlayWave(deadSE_,0.3f);

}

PlayerDeath ::~PlayerDeath() {

}

//更新
void PlayerDeath::Update() {

	switch (step_)
	{
	case PlayerDeath::Step::DEATH:
		///-------------------------------------------------------
		/// 死
		///-------------------------------------------------------
		 
		// 死亡前動作とか入れたければ 
		
		pPlayer_->SetScale(Vector3::GetZeroVec());
		pPlayer_->GetCollider()->SetIsCollisonCheck(false);
		pPlayer_->GetKikCollider()->SetIsCollisonCheck(false);

		step_ = Step::WAIT;
		break;
	case PlayerDeath::Step::WAIT:
		///-------------------------------------------------------
		///　待機
		///-------------------------------------------------------
		
		waitTime_ += FPSKeeper::NormalDeltaTime();
		if (waitTime_ < pPlayer_->GetParamater().respownWaitTime_)break;
		step_ = Step::GORESPOWN;

		break;
	case PlayerDeath::Step::GORESPOWN:
		///-------------------------------------------------------
		///　リスポーン
		///-------------------------------------------------------
		pPlayer_->ChangeState(std::make_unique<PlayerReSpown>(pPlayer_));
		break;
	default:
		break;
	}
	
	
}



void  PlayerDeath::Debug() {
#ifdef _DEBUG
	ImGui::Text("PlayerDeath");
#endif // _DEBUG	
}

