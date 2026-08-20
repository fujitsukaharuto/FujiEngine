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
	pBoss_->SetCameraRange(cameraRange_);
	pBoss_->SetCameraFollowSpeed(cameraFollowSpeed_);
	pBoss_->GetAnimeModel()->ChangeAnimation("punch");
	pBoss_->GetAnimeModel()->IsLoopAnimation(false);
	pBoss_->ChainCount();
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
			pBoss_->GetAnimeModel()->IsLoopAnimation(true);
			AudioPlayer::GetInstance()->SoundPlayWave(*attackSE_, 0.35f);
			pBoss_->WaveWallAttack();
			attackCount_++;
		} else {
			pBoss_->GetAnimeModel()->IsLoopAnimation(false);
			coolTime_ -= FPSKeeper::DeltaTimeFrame();
		}

		break;
		///---------------------------------------------------------------------------------------
		/// 通常へ移行
		///---------------------------------------------------------------------------------------
	case BossSwordAttack::Step::TOROOT:
		pBoss_->GetAnimeModel()->IsLoopAnimation(true);
		pBoss_->ChangeBehavior(std::make_unique<BossRoot>(pBoss_));
		break;
	default:
		break;
	}

}

void BossSwordAttack::Debug() {
}
