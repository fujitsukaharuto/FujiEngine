#include "BossAreaAttack.h"

#include "Game/GameObj/Enemy/Boss.h"
#include "BossRoot.h"

BossAreaAttack::BossAreaAttack(Boss* pBoss) : BaseBossBehavior(pBoss) {
	step_ = Step::ATTACK;
	pBoss_->GetAnimModel()->ChangeAnimation("swordLeft");
	pBoss_->GetAnimModel()->IsRoopAnimation(false);
}

BossAreaAttack::~BossAreaAttack() {
}

void BossAreaAttack::Update() {

	switch (step_) {
		///---------------------------------------------------------------------------------------
		/// 通常
		///---------------------------------------------------------------------------------------
	case BossAreaAttack::Step::ATTACK:

		if (isAttack_) {
			pBoss_->WaveWallAttack();
			isAttack_ = false;
		}
		coolTime_ -= FPSKeeper::DeltaTime();
		if (coolTime_ < 0.0f) {
			step_ = Step::TOROOT;
		}

		break;
		///---------------------------------------------------------------------------------------
		/// ジャンプへ移行
		///---------------------------------------------------------------------------------------
	case BossAreaAttack::Step::TOROOT:
		pBoss_->GetAnimModel()->IsRoopAnimation(true);
		pBoss_->ChangeBehavior(std::make_unique<BossRoot>(pBoss_));
		break;
	default:
		break;
	}

}

void BossAreaAttack::Debug() {
}
