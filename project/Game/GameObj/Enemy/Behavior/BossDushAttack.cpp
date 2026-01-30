#include "BossDushAttack.h"

#include "Engine/Math/Random/Random.h"
#include "Engine/Audio/AudioPlayer.h"
#include "Game/GameObj/Enemy/Boss.h"
#include "Game/GameObj/Enemy/Behavior/BossArrowAttack.h"
#include "BossRoot.h"

using namespace Core;
using namespace Graphics;
using namespace Math;


BossDushAttack::BossDushAttack(Boss* pBoss,bool is) : BaseBossBehavior(pBoss) {
	step_ = Step::CHARGE;
	cameraRange_ = -40.0f;
	cameraFollowSpeed_ = 0.03f;
	pBoss_->SetCameraRange(cameraRange_);
	pBoss_->SetCameraFollowSpeed(cameraFollowSpeed_);
	pBoss_->GetAnimeModel()->ChangeAnimation("DushPose");
	pBoss_->GetAnimeModel()->IsLoopAnimation(false);
	pBoss_->ChainCount();
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

		if (pBoss_->DushCharge(chargeTime_, maxChargeTime_, isNear_, stopRange_)) {
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
			if (pBoss_->DushAttack(isNear_, dushRange_, stopRange_)) {
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
		pBoss_->GetAnimeModel()->IsLoopAnimation(true);
		float randomSeed = Random::GetFloat(0.0f, 1.0f);
		if (randomSeed > pBoss_->GetChainRate() && !isPreDush_) {
			pBoss_->ChangeBehavior(std::make_unique<BossDushAttack>(pBoss_, true));
		} else {
			pBoss_->ChangeBehavior(std::make_unique<BossRoot>(pBoss_));
		}
		break;
	}
	default:
		break;
	}

}

void BossDushAttack::Debug() {
}
