#include "BossAttack.h"

#include "Game/GameObj/Enemy/Boss.h"
#include "BossRoot.h"

BossAttack::BossAttack(Boss* pBoss) : BaseBossBehavior(pBoss) {
	step_ = Step::ATTACK;

}

BossAttack::~BossAttack() {
}

void BossAttack::Update() {

	switch (step_) {
		///---------------------------------------------------------------------------------------
		/// 通常
		///---------------------------------------------------------------------------------------
	case BossAttack::Step::ATTACK:




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
