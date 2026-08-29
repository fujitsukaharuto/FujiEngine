#include "BossDushAttack.h"

#include "Engine/Math/Random/Random.h"
#include "Engine/Audio/AudioPlayer.h"
#include "Game/GameObj/Enemy/Boss.h"
#include "Game/GameObj/Enemy/Behavior/BossArrowAttack.h"
#include "BossRoot.h"
#include "Engine/Core/Time/FPSKeeper.h"

using namespace Core;
using namespace Graphics;
using namespace Math;


BossDushAttack::BossDushAttack(Boss* pBoss,bool is) : BaseBossBehavior(pBoss) {
	step_ = Step::CHARGE;
	cameraRange_ = -40.0f;
	cameraFollowSpeed_ = 0.03f;
	owner_->SetCameraRange(cameraRange_);
	owner_->SetCameraFollowSpeed(cameraFollowSpeed_);
	owner_->GetAnimeModel()->ChangeAnimation("DushPose");
	owner_->GetAnimeModel()->IsLoopAnimation(false);
	owner_->ChainCount();
	isPreDush_ = is;
	if (is) {
		coolTime_ = 60.0f;
	}
}

BossDushAttack::~BossDushAttack() {
}

void BossDushAttack::Update() {

	switch (step_) {
	case BossDushAttack::Step::CHARGE:

		if (owner_->DushCharge(chargeTime_, maxChargeTime_, isNear_, stopRange_)) {
			step_ = Step::ATTACK;
		}

		break;
		///---------------------------------------------------------------------------------------
		/// 攻撃
		///---------------------------------------------------------------------------------------
	case BossDushAttack::Step::ATTACK:
		if (startWaitTime_ < maxStartWaitTime_) {
			startWaitTime_ += FPSKeeper::DeltaTimeFrame();
			break;
		}

		if (isAttack_) {
			if (owner_->DushAttack(isNear_, dushRange_, stopRange_)) {
				isAttack_ = false;
			}
		}
		if (!isAttack_) {
			coolTime_ -= FPSKeeper::DeltaTimeFrame();
			if (coolTime_ < 0.0f) {
				step_ = Step::TOROOT;
			}
		}

		break;
		///---------------------------------------------------------------------------------------
		/// 通常or攻撃へ移行
		///---------------------------------------------------------------------------------------
	case BossDushAttack::Step::TOROOT:
	{
		owner_->GetAnimeModel()->IsLoopAnimation(true);
		float randomSeed = Random::GetFloat(0.0f, 1.0f);
		if (randomSeed > owner_->GetChainRate() && !isPreDush_) {
			owner_->ChangeBehavior(std::make_unique<BossDushAttack>(owner_, true));
		} else {
			owner_->ChangeBehavior(std::make_unique<BossRoot>(owner_));
		}
		break;
	}
	default:
		break;
	}

}
