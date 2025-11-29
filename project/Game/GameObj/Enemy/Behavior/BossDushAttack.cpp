#include "BossDushAttack.h"

#include "Engine/Math/Random/Random.h"
#include "Engine/Audio/AudioPlayer.h"
#include "Game/GameObj/Enemy/Boss.h"
#include "Game/GameObj/Enemy/Behavior/BossArrowAttack.h"
#include "BossRoot.h"

BossDushAttack::BossDushAttack(Boss* pBoss,bool is) : BaseBossBehavior(pBoss) {
	step_ = Step::CHARGE;
	cameraRang_ = -40.0f;
	cameraFollowSpeed_ = 0.03f;
	pBoss_->SetCameraRang(cameraRang_);
	pBoss_->SetCameraFollowSpeed(cameraFollowSpeed_);
	pBoss_->GetAnimModel()->ChangeAnimation("DushPose");
	pBoss_->GetAnimModel()->IsRoopAnimation(false);
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

		if (pBoss_->DushCharge(chargeTime_, maxCharegeTime_, isNear_, stopReng_)) {
			step_ = Step::ATTACK;
		}

		break;
		///---------------------------------------------------------------------------------------
		/// 攻撃
		///---------------------------------------------------------------------------------------
	case BossDushAttack::Step::ATTACK:
		if (startWaitTime_ < maxStartWaitTime_) {
			startWaitTime_ += FPSKeeper::DeltaTime();
			break;
		}

		if (isAttack_) {
			if (pBoss_->DushAttack(isNear_, dushReng_, stopReng_)) {
				isAttack_ = false;
			}
		}
		if (!isAttack_) {
			coolTime_ -= FPSKeeper::DeltaTime();
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
		pBoss_->GetAnimModel()->IsRoopAnimation(true);
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
