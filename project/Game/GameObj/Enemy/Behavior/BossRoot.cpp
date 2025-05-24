#include "BossRoot.h"

#include "Game/GameObj/Enemy/Boss.h"
#include "BossAttack.h"

BossRoot::BossRoot(Boss* pBoss) : BaseBossBehavior(pBoss) {
	step_ = Step::ROOT;
	cooldown_ = pBoss_->GetAttackCooldown();
}

BossRoot::~BossRoot() {
}

void BossRoot::Update() {

	switch (step_) {
		///---------------------------------------------------------------------------------------
		/// 通常
		///---------------------------------------------------------------------------------------
	case BossRoot::Step::ROOT:
		if (cooldown_ > 0.0f) {
			cooldown_ -= FPSKeeper::DeltaTime();
		} else if (cooldown_ <= 0.0f) {
			step_ = Step::TOATTACK;
			break;
		}


		break;
		///---------------------------------------------------------------------------------------
		/// 攻撃へ移行
		///---------------------------------------------------------------------------------------
	case BossRoot::Step::TOATTACK:
		pBoss_->ChangeBehavior(std::make_unique<BossAttack>(pBoss_));
		break;
	default:
		break;
	}

}

void BossRoot::Debug() {
}
