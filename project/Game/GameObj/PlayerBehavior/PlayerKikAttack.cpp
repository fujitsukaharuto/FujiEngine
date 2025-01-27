/// behavior
#include"PlayerKikAttack.h"
#include"PlayerAttackRoot.h"


/// boss
#include"GameObj/Player/Player.h"

/// frame
#include"DX/FPSKeeper.h"
/// inupt
#include<imgui.h>
#include<numbers>

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
        /// 1:キック
        ///------------------------------------------------------------
		kikTime_ += FPSKeeper::DeltaTimeRate();
		// タイム超越で次のステップ
		if (kikTime_ >=pPlayer_->GetParamater().kikTime_) {
			pPlayer_->SetKikIsCollision(false);
		};

		// 回転
		KikRotation();
		
		// タイム超越で次のステップ
		if (kikTime_ < pPlayer_->GetParamater().kikRotateTime_)break;
		pPlayer_->SetRotationZ(0.0f);
		step_ = Step::RETUNROOT;
		
		break;
	case Step::RETUNROOT:
		///-----------------------------------------------------------
		/// 2:通常に戻る
		///------------------------------------------------------------
	
		pPlayer_->ChangeAttackBehavior(std::make_unique<PlayerAttackRoot>(pPlayer_));
	
		break;
	default:
		break;
	}

}

void  PlayerKikAttack::Debug() {
	
}


void PlayerKikAttack::KikRotation() {
	if (pPlayer_->GetFacingDirection() == 1.0f) {
		kikRotateZ_ = EaseOutBack(0.0f, std::numbers::pi_v<float>*2.0f, kikTime_, pPlayer_->GetParamater().kikRotateTime_);
		pPlayer_->SetRotationZ(kikRotateZ_);
	}
	else {
		kikRotateZ_ = EaseOutBack(0.0f, -std::numbers::pi_v<float>*2.0f, kikTime_, pPlayer_->GetParamater().kikRotateTime_);
		pPlayer_->SetRotationZ(kikRotateZ_);
	}
}