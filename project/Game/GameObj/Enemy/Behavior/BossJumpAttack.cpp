#include "BossJumpAttack.h"

#include "Game/GameObj/Enemy/Boss.h"
#include "BossRoot.h"

BossJumpAttack::BossJumpAttack(Boss* pBoss, int count) : BaseBossBehavior(pBoss), jumpCount_(count) {
	step_ = Step::ATTACK;
	pBoss_->InitJumpAttack();
	pBoss_->GetAnimModel()->ChangeAnimation("jump");
	pBoss_->GetAnimModel()->IsRoopAnimation(false);
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
			pBoss_->GetAnimModel()->IsRoopAnimation(false);
			if (jumpCount_ == nowJumpCount_) {
				step_ = Step::TOROOT;
			} else {
				pBoss_->InitJumpAttack();
				pBoss_->GetAnimModel()->IsRoopAnimation(true);
			}
		}

		break;
		///---------------------------------------------------------------------------------------
		/// ジャンプへ移行
		///---------------------------------------------------------------------------------------
	case BossJumpAttack::Step::TOROOT:
		pBoss_->GetAnimModel()->IsRoopAnimation(true);
		pBoss_->ChangeBehavior(std::make_unique<BossRoot>(pBoss_));
		break;
	default:
		break;
	}

}

void BossJumpAttack::Debug() {
}
