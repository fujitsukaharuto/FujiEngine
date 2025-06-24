#include "BossAttack.h"

#include "Game/GameObj/Enemy/Boss.h"
#include "BossRoot.h"

BossAttack::BossAttack(Boss* pBoss) : BaseBossBehavior(pBoss) {
	step_ = Step::ATTACK;
	pBoss_->GetAnimModel()->ChangeAnimation("punch");
}

BossAttack::~BossAttack() {
}

void BossAttack::Update() {

	switch (step_) {
		///---------------------------------------------------------------------------------------
		/// 通常
		///---------------------------------------------------------------------------------------
	case BossAttack::Step::ATTACK:

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
	case BossAttack::Step::TOROOT:
		pBoss_->ChangeBehavior(std::make_unique<BossRoot>(pBoss_));
		break;
	default:
		break;
	}

}

void BossAttack::Debug() {
}
