#include "BossSwordAttack.h"

#include "Game/GameObj/Enemy/Boss.h"
#include "BossRoot.h"

BossSwordAttack::BossSwordAttack(Boss* pBoss) : BaseBossBehavior(pBoss) {
	step_ = Step::ATTACK;
	coolTime_ = 0.0f;
	attackCount_ = 0;
	pBoss_->GetAnimModel()->ChangeAnimation("punch");
}

BossSwordAttack::~BossSwordAttack() {
}

void BossSwordAttack::Update() {

	switch (step_) {
		///---------------------------------------------------------------------------------------
		/// 通常
		///---------------------------------------------------------------------------------------
	case BossSwordAttack::Step::ATTACK:

		if (coolTime_ <= 0.0f) {
			coolTime_ = 60.0f;
			pBoss_->WaveWallAttack();
			attackCount_++;
			if (attackCount_ >= 3) {
				step_ = Step::TOROOT;
			}
		} else {
			coolTime_ -= FPSKeeper::DeltaTime();
		}

		break;
		///---------------------------------------------------------------------------------------
		/// ジャンプへ移行
		///---------------------------------------------------------------------------------------
	case BossSwordAttack::Step::TOROOT:
		pBoss_->ChangeBehavior(std::make_unique<BossRoot>(pBoss_));
		break;
	default:
		break;
	}

}

void BossSwordAttack::Debug() {
}
