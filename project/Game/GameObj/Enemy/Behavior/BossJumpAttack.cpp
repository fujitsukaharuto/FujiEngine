#include "BossJumpAttack.h"

#include "Game/GameObj/Enemy/Boss.h"
#include "BossRoot.h"

BossJumpAttack::BossJumpAttack(Boss* pBoss, int count) : BaseBossBehavior(pBoss), jumpCount_(count) {
	step_ = Step::ATTACK;
	pBoss_->InitJumpAttack();
}

BossJumpAttack::~BossJumpAttack() {
}

void BossJumpAttack::Update() {

	switch (step_) {
		///---------------------------------------------------------------------------------------
		/// 通常
		///---------------------------------------------------------------------------------------
	case BossJumpAttack::Step::ATTACK:

		if (pBoss_->JumpAttack()) {
			nowJumpCount_++;
			if (jumpCount_ == nowJumpCount_) {
				step_ = Step::TOROOT;
			} else {
				pBoss_->InitJumpAttack();
			}
		}

		break;
		///---------------------------------------------------------------------------------------
		/// ジャンプへ移行
		///---------------------------------------------------------------------------------------
	case BossJumpAttack::Step::TOROOT:
		pBoss_->ChangeBehavior(std::make_unique<BossRoot>(pBoss_));
		break;
	default:
		break;
	}

}

void BossJumpAttack::Debug() {
}
