#include "BossArrowAttack.h"

#include "Engine/Math/Random/Random.h"
#include "Game/GameObj/Enemy/Boss.h"
#include "Game/GameObj/Enemy/Behavior/BossSwordAttack.h"
#include "Game/GameObj/Enemy/Behavior/BossAreaAttack.h"
#include "BossRoot.h"
#include "Engine/Core/Time/FPSKeeper.h"

using namespace Core;
using namespace Graphics;
using namespace Math;


BossArrowAttack::BossArrowAttack(Boss* pBoss,bool beforArrow) : BaseBossBehavior(pBoss) {
	step_ = Step::ATTACK;
	owner_->SetCameraRange(cameraRange_);
	owner_->SetCameraFollowSpeed(cameraFollowSpeed_);
	owner_->GetAnimeModel()->ChangeAnimation("idle");
	owner_->GetAnimeModel()->IsLoopAnimation(false);
	isbeforArrow_ = beforArrow; // 1つ前がarrowだったら次はarrowにならないように
	if (beforArrow) {
		beforWait_ = 60.0f;
		coolTime_ = 30.0f;
	}
	owner_->ChainCount();
}

BossArrowAttack::~BossArrowAttack() {
}

void BossArrowAttack::Update() {

	switch (step_) {
		///---------------------------------------------------------------------------------------
		/// 攻撃
		///---------------------------------------------------------------------------------------
	case BossArrowAttack::Step::ATTACK:

		if (isAttack_ && beforWait_ <= 0.0f) {
			owner_->ArrowAttack();
			isAttack_ = false;
		}
		beforWait_ -= FPSKeeper::DeltaTimeFrame();
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
	case BossArrowAttack::Step::TOROOT:
	{
		owner_->GetAnimeModel()->IsLoopAnimation(true);
		float randomSeed = Random::GetFloat(0.0f, 1.0f);
		if (randomSeed > owner_->GetChainRate() + 0.05f && !isbeforArrow_) {
			owner_->ChangeBehavior(std::make_unique<BossArrowAttack>(owner_, true));
		} else if (randomSeed > owner_->GetChainRate() - 0.15f && randomSeed < owner_->GetChainRate() + 0.05f) {
			owner_->ChangeBehavior(std::make_unique<BossAreaAttack>(owner_));
		} else if (isbeforArrow_ && randomSeed > owner_->GetChainRate() - 0.3f) {
			owner_->ChangeBehavior(std::make_unique<BossSwordAttack>(owner_));
		} else {
			owner_->ChangeBehavior(std::make_unique<BossRoot>(owner_));
		}
		break;
	}
	default:
		break;
	}

}
