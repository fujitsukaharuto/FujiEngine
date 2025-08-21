#include "BossAttack.h"

#include "Engine/Math/Random/Random.h"
#include "Game/GameObj/Enemy/Boss.h"
#include "Game/GameObj/Enemy/Behavior/BossArrowAttack.h"
#include "BossRoot.h"

BossAttack::BossAttack(Boss* pBoss) : BaseBossBehavior(pBoss) {
	step_ = Step::ATTACK;
	pBoss_->GetAnimModel()->ChangeAnimation("punch");
	pBoss_->GetAnimModel()->IsRoopAnimation(false);
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
	{
		pBoss_->GetAnimModel()->IsRoopAnimation(true);
		float randomSeed = Random::GetFloat(0.0f, 1.0f);
		if (randomSeed > pBoss_->GetChainRate()) {
			pBoss_->ChangeBehavior(std::make_unique<BossArrowAttack>(pBoss_));
		} else {
			pBoss_->ChangeBehavior(std::make_unique<BossRoot>(pBoss_));
		}
		break;
	}
	default:
		break;
	}

}

void BossAttack::Debug() {
}
