/// behavior
#include"PlayerKikAttack.h"
#include"PlayerAttackRoot.h"
#include"PlayerRecoil.h"

/// boss
#include"GameObj/Player/Player.h"
#include"GameObj/Enemy/BaseEnemy.h"
/// frame
#include"DX/FPSKeeper.h"
/// inupt
#include"Input/Input.h"
#include"GameObj/JoyState/JoyState.h"

#include<imgui.h>

//初期化
PlayerKikAttack::PlayerKikAttack(Player* boss)
	: BasePlayerAttackBehavior("PlayerKikAttack", boss) {

	/// ===================================================
	/// 変数初期化
	/// ===================================================
	pPlayer_->SetKikIsCollision(true);
	step_ = Step::KIK;
}

PlayerKikAttack ::~PlayerKikAttack() {

}

//更新
void PlayerKikAttack::Update() {
	

	//pPlayer_->Move(pPlayer_->GetJumpSpeed());

	switch (step_)
	{
	case Step::KIK:
		///-----------------------------------------------------------
        /// キック
        ///------------------------------------------------------------
		kikTime_ += FPSKeeper::DeltaTimeRate();
		
		if (kikTime_ < pPlayer_->GetParamater().kikTime_) break;// キックタイム
		step_ = Step::RETUNROOT;
		
		break;
	case Step::RETUNROOT:
		///-----------------------------------------------------------
		/// 通常に戻る
		///------------------------------------------------------------
		pPlayer_->SetKikIsCollision(false);
		pPlayer_->ChangeAttackBehavior(std::make_unique<PlayerAttackRoot>(pPlayer_));
	
		break;
	default:
		break;
	}

}

void  PlayerKikAttack::Debug() {
	
}


