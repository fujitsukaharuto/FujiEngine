#include "BossRodFall.h"

#include "Engine/Math/Random/Random.h"
#include "Game/GameObj/Enemy/Boss.h"
#include "BossRoot.h"
#include "Engine/Core/Time/FPSKeeper.h"

using namespace Core;
using namespace Graphics;


BossRodFall::BossRodFall(Boss* pBoss) : BaseBossBehavior(pBoss) {
	step_ = Step::ATTACK;
	cameraRange_ = -35.0f;
	owner_->SetCameraRange(cameraRange_);
	owner_->SetCameraFollowSpeed(cameraFollowSpeed_);
	owner_->GetAnimeModel()->ChangeAnimation("idle");
	owner_->GetAnimeModel()->IsLoopAnimation(false);
	owner_->ChainCount();
}

BossRodFall::~BossRodFall() {
}

void BossRodFall::Update() {

	switch (step_) {
		///---------------------------------------------------------------------------------------
		/// 攻撃
		///---------------------------------------------------------------------------------------
	case BossRodFall::Step::ATTACK:

		if (isAttack_) {
			owner_->RodFall();
			isAttack_ = false;
			isChange_ = true;
		}
		if (!isAttack_) {
			coolTime_ -= FPSKeeper::DeltaTimeFrame();
			if (coolTime_ < 100.0f && isChange_) {
				isChange_ = false;
				owner_->GetAnimeModel()->ChangeAnimation("swordLeft");
			}
			if (coolTime_ < 0.0f) {
				step_ = Step::TOROOT;
			}
		}

		break;
		///---------------------------------------------------------------------------------------
		/// 通常へ移行
		///---------------------------------------------------------------------------------------
	case BossRodFall::Step::TOROOT:
	{
		owner_->GetAnimeModel()->IsLoopAnimation(true);
		owner_->ChangeBehavior(std::make_unique<BossRoot>(owner_));
		break;
	}
	default:
		break;
	}

}
