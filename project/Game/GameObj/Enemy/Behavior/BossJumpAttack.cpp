#include "BossJumpAttack.h"

#include "Engine/Math/Random/Random.h"
#include "Game/GameObj/Enemy/Boss.h"
#include "Game/GameObj/Enemy/Behavior/BossAttack.h"
#include "BossRoot.h"

using namespace Graphics;
using namespace Math;


BossJumpAttack::BossJumpAttack(Boss* pBoss, int count) : BaseBossBehavior(pBoss), jumpCount_(count) {
	step_ = Step::ATTACK;
	cameraRange_ = -35.0f;
	pBoss_->SetCameraRange(cameraRange_);
	pBoss_->SetCameraFollowSpeed(cameraFollowSpeed_);
	pBoss_->InitJumpAttack();
	pBoss_->GetAnimeModel()->ChangeAnimation("jump");
	pBoss_->GetAnimeModel()->IsLoopAnimation(false);
	pBoss_->ChainCount();
}

BossJumpAttack::~BossJumpAttack() {
}

void BossJumpAttack::Update() {

	switch (step_) {
		///---------------------------------------------------------------------------------------
		/// ジャンプ攻撃
		///---------------------------------------------------------------------------------------
	case BossJumpAttack::Step::ATTACK:

		if (pBoss_->JumpAttack()) {
			nowJumpCount_++;
			pBoss_->GetAnimeModel()->IsLoopAnimation(false);
			if (jumpCount_ == nowJumpCount_) {
				step_ = Step::TOROOT;
			} else {
				pBoss_->InitJumpAttack();
				pBoss_->GetAnimeModel()->IsLoopAnimation(true);
			}
		}

		break;
		///---------------------------------------------------------------------------------------
		/// 通常or攻撃へ移行
		///---------------------------------------------------------------------------------------
	case BossJumpAttack::Step::TOROOT:
	{
		pBoss_->GetAnimeModel()->IsLoopAnimation(true);
		float randomSeed = Random::GetFloat(0.0f, 1.0f);
		if (randomSeed > pBoss_->GetChainRate()) {
			pBoss_->ChangeBehavior(std::make_unique<BossAttack>(pBoss_));
		} else {
			pBoss_->ChangeBehavior(std::make_unique<BossRoot>(pBoss_));
		}
	}
		break;
	default:
		break;
	}

}

void BossJumpAttack::Debug() {
}
