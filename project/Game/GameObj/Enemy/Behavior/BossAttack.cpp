#include "BossAttack.h"

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


BossAttack::BossAttack(Boss* pBoss) : BaseBossBehavior(pBoss) {
	step_ = Step::ATTACK;
	owner_->SetCameraRange(cameraRange_);
	owner_->SetCameraFollowSpeed(cameraFollowSpeed_);
	owner_->GetAnimeModel()->ChangeAnimation("punch");
	owner_->GetAnimeModel()->IsLoopAnimation(false);
	owner_->ChainCount();
	attackSE_ = &AudioPlayer::GetInstance()->SoundLoadWave("attackSE.wav");
}

BossAttack::~BossAttack() {
}

void BossAttack::Update() {

	switch (step_) {
		///---------------------------------------------------------------------------------------
		/// 攻撃
		///---------------------------------------------------------------------------------------
	case BossAttack::Step::ATTACK:

		if (isAttack_) {
			owner_->WaveWallAttack();
			AudioPlayer::GetInstance()->SoundPlayWave(*attackSE_, 0.35f);
			isAttack_ = false;
		}
		coolTime_ -= FPSKeeper::DeltaTimeFrame();
		if (coolTime_ < 0.0f) {
			step_ = Step::TOROOT;
		}

		break;
		///---------------------------------------------------------------------------------------
		/// 通常or攻撃へ移行
		///---------------------------------------------------------------------------------------
	case BossAttack::Step::TOROOT:
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
