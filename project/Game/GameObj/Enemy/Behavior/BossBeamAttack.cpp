#include "BossBeamAttack.h"

#include "Game/GameObj/Enemy/Boss.h"
#include "BossRoot.h"

BossBeamAttack::BossBeamAttack(Boss* pBoss) : BaseBossBehavior(pBoss) {
	step_ = Step::CHARGE;

	pBoss_->InitBeam();
	pBoss_->GetAnimModel()->ChangeAnimation("idle");
}

BossBeamAttack::~BossBeamAttack() {
}

void BossBeamAttack::Update() {

	switch (step_) {
	case BossBeamAttack::Step::CHARGE:

		if (pBoss_->BeamCharge()) {
			pBoss_->BeamChargeComplete();
			step_ = Step::ATTACK;
		}

		break;
		///---------------------------------------------------------------------------------------
		/// 通常
		///---------------------------------------------------------------------------------------
	case BossBeamAttack::Step::ATTACK:

		if (pBoss_->BeamAttack()) {
			step_ = Step::TOROOT;
		}

		break;
		///---------------------------------------------------------------------------------------
		/// ジャンプへ移行
		///---------------------------------------------------------------------------------------
	case BossBeamAttack::Step::TOROOT:
		pBoss_->ChangeBehavior(std::make_unique<BossRoot>(pBoss_));
		break;
	default:
		break;
	}

}

void BossBeamAttack::Debug() {
}
