/// behavior
#include"PlayerReSpown.h"
#include"PlayerNoneState.h"
#include"GameObj/PlayerBehavior/PlayerAttackRoot.h"
#include"GameObj/PlayerBehavior/PlayerRoot.h"
/// boss
#include"GameObj/Player/Player.h"
/// frame
#include"DX/FPSKeeper.h"

#include<imgui.h>

//初期化
PlayerReSpown::PlayerReSpown(Player* boss)
	: BasePlayerState("PlayerReSpown", boss) {

	/// ===================================================
	/// 変数初期化
	/// ===================================================

	// パラメータ、位置リセット
	pPlayer_->SetdeathCount(pPlayer_->GetParamater().deathCount_);
	pPlayer_->SetDamageRenditionReset();
	pPlayer_->SetWorldPosition(pPlayer_->GetParamater().respownPos_);

	// 振る舞いリセット
	pPlayer_->ChangeBehavior(std::make_unique<PlayerRoot>(pPlayer_));
	pPlayer_->ChangeAttackBehavior(std::make_unique<PlayerAttackRoot>(pPlayer_));

	ease_.maxTime = 0.7f;
	step_ = Step::RESPAWN;
}

PlayerReSpown ::~PlayerReSpown() {

}

//更新
void PlayerReSpown::Update() {

	switch (step_)
	{
	case PlayerReSpown::Step::RESPAWN:
		///-------------------------------------------------------
		/// 復活
		///-------------------------------------------------------
		ease_.time += FPSKeeper::NormalDeltaTime();
		invincibleTime_ += FPSKeeper::NormalDeltaTime();

		pPlayer_->DamageRendition(0.1f);// ダメージ演出

		ease_.time = min(ease_.time, ease_.maxTime);
		if (ease_.time >= ease_.maxTime) {
			ease_.time = ease_.maxTime;
			pPlayer_->SetScale(Vector3(1, 1, 1));
		}
		pPlayer_->SetScale(EaseOutBack(Vector3::GetZeroVec(), Vector3(1, 1, 1), ease_.time, ease_.maxTime));

		if (invincibleTime_ < pPlayer_->GetParamater().respownInvincibleTime_)break;
		pPlayer_->GetModel()->SetColor(Vector4(1, 1, 1, 1));
		pPlayer_->GetCollider()->SetIsCollisonCheck(true);
		step_ = Step::GOROOT;
		break;
	case PlayerReSpown::Step::GOROOT:
		///-------------------------------------------------------
		/// 死
		///-------------------------------------------------------
		pPlayer_->ChangeState(std::make_unique<PlayerNoneState>(pPlayer_));
		break;
	default:
		break;
	}

}



void  PlayerReSpown::Debug() {
#ifdef _DEBUG
	ImGui::Text("PlayerReSpown");
#endif // _DEBUG	
}

