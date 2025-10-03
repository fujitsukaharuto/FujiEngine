#include "BossArrowAttack.h"

#include "Engine/Math/Random/Random.h"
#include "Game/GameObj/Enemy/Boss.h"
#include "Game/GameObj/Enemy/Behavior/BossSwordAttack.h"
#include "Game/GameObj/Enemy/Behavior/BossAreaAttack.h"
#include "BossRoot.h"

BossArrowAttack::BossArrowAttack(Boss* pBoss,bool beforArrow) : BaseBossBehavior(pBoss) {
	step_ = Step::ATTACK;
	pBoss_->GetAnimModel()->ChangeAnimation("idle");
	pBoss_->GetAnimModel()->IsRoopAnimation(false);
	isbeforArrow_ = beforArrow; // 1つ前がarrowだったら次はarrowにならないように
	if (beforArrow) {
		beforWait_ = 60.0f;
		coolTime_ = 30.0f;
	}
	pBoss_->ChainCount();
}

BossArrowAttack::~BossArrowAttack() {
}

void BossArrowAttack::Update() {

	switch (step_) {
		///---------------------------------------------------------------------------------------
		/// 攻撃
		///---------------------------------------------------------------------------------------
	case BossArrowAttack::Step::ATTACK:

		if (isAttack_ && beforWait_ <= 0.0f) {
			pBoss_->ArrowAttack();
			isAttack_ = false;
		}
		beforWait_ -= FPSKeeper::DeltaTime();
		if (!isAttack_) {
			coolTime_ -= FPSKeeper::DeltaTime();
			if (coolTime_ < 0.0f) {
				step_ = Step::TOROOT;
			}
		}

		break;
		///---------------------------------------------------------------------------------------
		/// 通常or攻撃へ移行
		///---------------------------------------------------------------------------------------
	case BossArrowAttack::Step::TOROOT:
	{
		pBoss_->GetAnimModel()->IsRoopAnimation(true);
		float randomSeed = Random::GetFloat(0.0f, 1.0f);
		if (randomSeed > pBoss_->GetChainRate() + 0.05f && !isbeforArrow_) {
			pBoss_->ChangeBehavior(std::make_unique<BossArrowAttack>(pBoss_, true));
		} else if (randomSeed > pBoss_->GetChainRate() - 0.15f && randomSeed < pBoss_->GetChainRate() + 0.05f) {
			pBoss_->ChangeBehavior(std::make_unique<BossAreaAttack>(pBoss_));
		} else if (isbeforArrow_ && randomSeed > pBoss_->GetChainRate() - 0.3f) {
			pBoss_->ChangeBehavior(std::make_unique<BossSwordAttack>(pBoss_));
		} else {
			pBoss_->ChangeBehavior(std::make_unique<BossRoot>(pBoss_));
		}
		break;
	}
	default:
		break;
	}

}

void BossArrowAttack::Debug() {
}
