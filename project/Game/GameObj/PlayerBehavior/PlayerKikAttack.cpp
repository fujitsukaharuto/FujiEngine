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
	pPlayer_->SetIsCollision(true);
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
		kikTime_ += FPSKeeper::NormalDeltaTime();
		
		if (kikTime_ < pPlayer_->GetKikTime()) break;// キックタイム
		step_ = Step::RETUNROOT;
		
		break;
	case Step::RETUNROOT:
		///-----------------------------------------------------------
		/// 通常に戻る
		///------------------------------------------------------------
		pPlayer_->SetIsCollision(false);
		pPlayer_->ChangeAttackBehavior(std::make_unique<PlayerAttackRoot>(pPlayer_));
	
		break;
	default:
		break;
	}

}

void  PlayerKikAttack::Debug() {
	
}


