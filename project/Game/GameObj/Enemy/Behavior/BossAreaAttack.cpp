#include "BossAreaAttack.h"

#include "Engine/Math/Random/Random.h"
#include "Game/GameObj/Enemy/Boss.h"
#include "Game/GameObj/Enemy/Behavior/BossArrowAttack.h"
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

		coolTime_ -= FPSKeeper::DeltaTime();
		if (coolTime_ < 0.0f) {
			if (isAttack_) {
				pBoss_->WaveWallAttack();
				pBoss_->UnderRingEmit();
				isAttack_ = false;
			} else {
				step_ = Step::TOROOT;
			}
			coolTime_ = 30.0f;
		}

		break;
		///---------------------------------------------------------------------------------------
		/// ジャンプへ移行
		///---------------------------------------------------------------------------------------
	case BossAreaAttack::Step::TOROOT:
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

void BossAreaAttack::Debug() {
}
