#include "BossRodFall.h"

#include "Engine/Math/Random/Random.h"
#include "Game/GameObj/Enemy/Boss.h"
#include "BossRoot.h"

BossRodFall::BossRodFall(Boss* pBoss) : BaseBossBehavior(pBoss) {
	step_ = Step::ATTACK;
	pBoss_->GetAnimModel()->ChangeAnimation("idle");
	pBoss_->GetAnimModel()->IsRoopAnimation(false);
}

BossRodFall::~BossRodFall() {
}

void BossRodFall::Update() {

	switch (step_) {
		///---------------------------------------------------------------------------------------
		/// 通常
		///---------------------------------------------------------------------------------------
	case BossRodFall::Step::ATTACK:

		if (isAttack_) {
			pBoss_->RodFall();
			isAttack_ = false;
			isChange_ = true;
		}
		if (!isAttack_) {
			coolTime_ -= FPSKeeper::DeltaTime();
			if (coolTime_ < 100.0f && isChange_) {
				isChange_ = false;
				pBoss_->GetAnimModel()->ChangeAnimation("swordLeft");
			}
			if (coolTime_ < 0.0f) {
				step_ = Step::TOROOT;
			}
		}

		break;
		///---------------------------------------------------------------------------------------
		/// ジャンプへ移行
		///---------------------------------------------------------------------------------------
	case BossRodFall::Step::TOROOT:
	{
		pBoss_->GetAnimModel()->IsRoopAnimation(true);
		pBoss_->ChangeBehavior(std::make_unique<BossRoot>(pBoss_));
		break;
	}
	default:
		break;
	}

}

void BossRodFall::Debug() {
}
