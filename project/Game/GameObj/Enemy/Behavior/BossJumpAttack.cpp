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
	owner_->SetCameraRange(cameraRange_);
	owner_->SetCameraFollowSpeed(cameraFollowSpeed_);
	owner_->InitJumpAttack();
	owner_->GetAnimeModel()->ChangeAnimation("jump");
	owner_->GetAnimeModel()->IsLoopAnimation(false);
	owner_->ChainCount();
}

BossJumpAttack::~BossJumpAttack() {
}

void BossJumpAttack::Update() {

	switch (step_) {
		///---------------------------------------------------------------------------------------
		/// ジャンプ攻撃
		///---------------------------------------------------------------------------------------
	case BossJumpAttack::Step::ATTACK:

		if (owner_->JumpAttack()) {
			nowJumpCount_++;
			owner_->GetAnimeModel()->IsLoopAnimation(false);
			if (jumpCount_ == nowJumpCount_) {
				step_ = Step::TOROOT;
			} else {
				owner_->InitJumpAttack();
				owner_->GetAnimeModel()->IsLoopAnimation(true);
			}
		}

		break;
		///---------------------------------------------------------------------------------------
		/// 通常or攻撃へ移行
		///---------------------------------------------------------------------------------------
	case BossJumpAttack::Step::TOROOT:
	{
		owner_->GetAnimeModel()->IsLoopAnimation(true);
		float randomSeed = Random::GetFloat(0.0f, 1.0f);
		if (randomSeed > owner_->GetChainRate()) {
			owner_->ChangeBehavior(std::make_unique<BossAttack>(owner_));
		} else {
			owner_->ChangeBehavior(std::make_unique<BossRoot>(owner_));
		}
	}
		break;
	default:
		break;
	}

}
