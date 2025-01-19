/// behavior
#include"PlayerSpecialFall.h"
#include"PlayerAttackRoot.h"
#include"PlayerRoot.h"
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
PlayerSpecialFall::PlayerSpecialFall(Player* boss)
	: BasePlayerAttackBehavior("PlayerSpecialFall", boss) {

	/// ===================================================
	/// 変数初期化
	/// ===================================================
	FPSKeeper::SetTimeScale(0.0f);
	postJumpSpeed_ = pPlayer_->GetParamater().specialAttackPostJump_;
	rotateSpeed_ = 30.0f;
	pPlayer_->SetKikIsCollision(false);
	step_ = Step::ANTIPATION;
	
}

PlayerSpecialFall ::~PlayerSpecialFall() {

}

//更新
void PlayerSpecialFall::Update() {
	switch (step_)
	{
	case PlayerSpecialFall::Step::ANTIPATION:
		/// ---------------------------------------------------------
        ///  予備動作
        /// ---------------------------------------------------------
		antiTime_ += FPSKeeper::NormalDeltaTime();

		if (antiTime_ < pPlayer_->GetParamater().specialAttackAntiTime_)break;
		step_ = Step::FALL;
		
		break;
	case PlayerSpecialFall::Step::FALL:
		/// ---------------------------------------------------------
		///  落ちる
		/// ---------------------------------------------------------
		pPlayer_->AddPosition(Vector3(
			0, 
			-pPlayer_->GetParamater().specialAttackFallSpeed_* FPSKeeper::NormalDeltaTime(),
			0));

		if (pPlayer_->GetModel()->GetWorldPos().y > Player::InitY_)break;
		pPlayer_->SetWorldPositionY(Player::InitY_);

		step_ = Step::POSTACTION;

		break;
	
	case PlayerSpecialFall::Step::POSTACTION:
		/// ---------------------------------------------------------
	    ///  後動作
	    /// ---------------------------------------------------------
		pPlayer_->Jump(postJumpSpeed_);
		rotationZ_ += rotateSpeed_ * FPSKeeper::NormalDeltaTime();
		pPlayer_->SetRotationZ(rotationZ_);

		if (pPlayer_->GetTrans().translate.y > Player::InitY_)break;
		pPlayer_->SetWorldPositionY(Player::InitY_);
		pPlayer_->SetRotationZ(0.0f);

		step_ = Step::RETURNROOT;

		break;
	case PlayerSpecialFall::Step::RETURNROOT:
		/// ---------------------------------------------------------
		///  通常に戻る
		/// ---------------------------------------------------------
		FPSKeeper::SetTimeScale(1.0f);
		pPlayer_->ChangeBehavior(std::make_unique<PlayerRoot>(pPlayer_));
		pPlayer_->ChangeAttackBehavior(std::make_unique<PlayerAttackRoot>(pPlayer_));
		
		break;
	default:
		break;
	}
	
	
}

void PlayerSpecialFall::KikAttackState() {
	
}


void  PlayerSpecialFall::Debug() {

}

