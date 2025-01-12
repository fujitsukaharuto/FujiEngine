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
	
	/// collider
	weakikCollider_ = std::make_unique<AABBCollider>();
	weakikCollider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	weakikCollider_->SetTag("WeakKik");
	weakikCollider_->SetParent(pPlayer_->GetModel());
	weakikCollider_->SetWidth(5.0f);
	weakikCollider_->SetHeight(5.0f);
	weakikCollider_->SetDepth(20.0f);
	weakikCollider_->SetPos(Vector3(0, 0, 1.5f));
	weakikCollider_->InfoUpdate();

	isCollision_ = false;
	step_ = Step::KIK;
}

PlayerKikAttack ::~PlayerKikAttack() {

}

//更新
void PlayerKikAttack::Update() {
	weakikCollider_->InfoUpdate();

	//pPlayer_->Move(pPlayer_->GetJumpSpeed());

	switch (step_)
	{
	case Step::KIK:
		///-----------------------------------------------------------
        /// キック
        ///------------------------------------------------------------
		kikTime_ += FPSKeeper::NormalDeltaTime();
		// 反動に移行
		if (isCollision_) {
			pPlayer_->ChangeBehavior(std::make_unique<PlayerRecoil>(pPlayer_));
			pPlayer_->ChangeAttackBehavior(std::make_unique<PlayerAttackRoot>(pPlayer_));
			break;
		}
		if (kikTime_ < pPlayer_->GetKikTime()) break;// キックタイム
		step_ = Step::RETUNROOT;
		
		break;
	case Step::RETUNROOT:
		///-----------------------------------------------------------
		/// 通常に戻る
		///------------------------------------------------------------
		pPlayer_->ChangeAttackBehavior(std::make_unique<PlayerAttackRoot>(pPlayer_));
		break;
	default:
		break;
	}

}

void  PlayerKikAttack::Debug() {
#ifdef _DEBUG
	weakikCollider_->DrawCollider();
#endif // _DEBUG	
}


void PlayerKikAttack::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {

	if (other.tag=="Enemy") {
		isCollision_ = true;
		return;
	}


}

void PlayerKikAttack::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {

}
