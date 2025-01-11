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
	
	weakikCollider_ = std::make_unique<AABBCollider>();
	weakikCollider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	weakikCollider_->SetTag("WeakKik");
	weakikCollider_->SetParent(pPlayer_->GetModel());
	weakikCollider_->SetWidth(1.0f);
	weakikCollider_->SetHeight(1.0f);
	weakikCollider_->SetDepth(1.0f);
	weakikCollider_->SetPos(Vector3(1, 0, 0));

	isCollision_ = false;
	step_ = Step::KIK;
}

PlayerKikAttack ::~PlayerKikAttack() {

}

//更新
void PlayerKikAttack::Update() {
	weakikCollider_->InfoUpdate();
	pPlayer_->Move(pPlayer_->GetJumpSpeed());

	switch (step_)
	{
	case Step::KIK:
		kikTime_ += FPSKeeper::NormalDeltaTime();
		if (isCollision_) {
			pPlayer_->ChangeBehavior(std::make_unique<PlayerRecoil>(pPlayer_));
			pPlayer_->ChangeAttackBehavior(std::make_unique<PlayerAttackRoot>(pPlayer_));
			break;
		}
		if (kikTime_ < 0.2f) break;
		step_ = Step::RETUNROOT;
		
		break;
	case Step::RETUNROOT:
		pPlayer_->ChangeAttackBehavior(std::make_unique<PlayerAttackRoot>(pPlayer_));
		break;
	default:
		break;
	}

	
	
}



void  PlayerKikAttack::Debug() {
#ifdef _DEBUG
	ImGui::Text("Root");
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
