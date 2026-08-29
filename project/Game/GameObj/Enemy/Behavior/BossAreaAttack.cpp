#include "BossAreaAttack.h"

#include "Engine/Math/Random/Random.h"
#include "Engine/Audio/AudioPlayer.h"
#include "Game/GameObj/Enemy/Boss.h"
#include "Game/GameObj/Enemy/Behavior/BossArrowAttack.h"
#include "BossRoot.h"
#include "Engine/Core/Time/FPSKeeper.h"

using namespace Audio;
using namespace Core;
using namespace Graphics;
using namespace Math;


BossAreaAttack::BossAreaAttack(Boss* pBoss) : BaseBossBehavior(pBoss) {
	step_ = Step::ATTACK;
	owner_->SetCameraRange(cameraRange_);
	owner_->SetCameraFollowSpeed(cameraFollowSpeed_);
	owner_->GetAnimeModel()->ChangeAnimation("swordLeft");
	owner_->GetAnimeModel()->IsLoopAnimation(false);
	owner_->ChainCount();
	areaAttackSE_ = &AudioPlayer::GetInstance()->SoundLoadWave("areaAttackSE.wav");
}

BossAreaAttack::~BossAreaAttack() {
}

void BossAreaAttack::Update() {

	switch (step_) {
		///---------------------------------------------------------------------------------------
		/// 攻撃
		///---------------------------------------------------------------------------------------
	case BossAreaAttack::Step::ATTACK:

		coolTime_ -= FPSKeeper::DeltaTimeFrame();
		if (coolTime_ < 0.0f) {
			if (isAttack_) {
				owner_->WaveWallAttack();
				owner_->UnderRingEmit();
				AudioPlayer::GetInstance()->SoundPlayWave(*areaAttackSE_, 0.35f);
				isAttack_ = false;
			} else {
				step_ = Step::TOROOT;
			}
			coolTime_ = 30.0f;
		}

		break;
		///---------------------------------------------------------------------------------------
		/// 通常or攻撃へ移行
		///---------------------------------------------------------------------------------------
	case BossAreaAttack::Step::TOROOT:
	{
		owner_->GetAnimeModel()->IsLoopAnimation(true);
		float randomSeed = Random::GetFloat(0.0f, 1.0f);
		if (randomSeed > owner_->GetChainRate()) {
			owner_->ChangeBehavior(std::make_unique<BossArrowAttack>(owner_));
		} else {
			owner_->ChangeBehavior(std::make_unique<BossRoot>(owner_));
		}
		break;
	}
	default:
		break;
	}

}
