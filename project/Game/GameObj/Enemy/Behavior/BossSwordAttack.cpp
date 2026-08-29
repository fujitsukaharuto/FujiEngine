#include "BossSwordAttack.h"

#include "Game/GameObj/Enemy/Boss.h"
#include "BossRoot.h"
#include "Engine/Core/Time/FPSKeeper.h"

using namespace Audio;
using namespace Core;
using namespace Graphics;


BossSwordAttack::BossSwordAttack(Boss* pBoss) : BaseBossBehavior(pBoss) {
	step_ = Step::ATTACK;
	coolTime_ = 0.0f;
	attackCount_ = 0;
	owner_->SetCameraRange(cameraRange_);
	owner_->SetCameraFollowSpeed(cameraFollowSpeed_);
	owner_->GetAnimeModel()->ChangeAnimation("punch");
	owner_->GetAnimeModel()->IsLoopAnimation(false);
	owner_->ChainCount();
	attackSE_ = &AudioPlayer::GetInstance()->SoundLoadWave("attackSE.wav");
}

BossSwordAttack::~BossSwordAttack() {
}

void BossSwordAttack::Update() {

	switch (step_) {
		///---------------------------------------------------------------------------------------
		/// 攻撃
		///---------------------------------------------------------------------------------------
	case BossSwordAttack::Step::ATTACK:

		if (coolTime_ <= 0.0f) {
			if (attackCount_ >= 3) {
				step_ = Step::TOROOT;
				break;
			}
			coolTime_ = 90.0f;
			owner_->GetAnimeModel()->IsLoopAnimation(true);
			AudioPlayer::GetInstance()->SoundPlayWave(*attackSE_, 0.35f);
			owner_->WaveWallAttack();
			attackCount_++;
		} else {
			owner_->GetAnimeModel()->IsLoopAnimation(false);
			coolTime_ -= FPSKeeper::DeltaTimeFrame();
		}

		break;
		///---------------------------------------------------------------------------------------
		/// 通常へ移行
		///---------------------------------------------------------------------------------------
	case BossSwordAttack::Step::TOROOT:
		owner_->GetAnimeModel()->IsLoopAnimation(true);
		owner_->ChangeBehavior(std::make_unique<BossRoot>(owner_));
		break;
	default:
		break;
	}

}
