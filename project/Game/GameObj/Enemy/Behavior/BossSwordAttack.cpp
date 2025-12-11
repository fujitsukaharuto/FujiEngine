#include "BossSwordAttack.h"

#include "Game/GameObj/Enemy/Boss.h"
#include "BossRoot.h"

using namespace Audio;
using namespace Core;
using namespace Graphics;


BossSwordAttack::BossSwordAttack(Boss* pBoss) : BaseBossBehavior(pBoss) {
	step_ = Step::ATTACK;
	coolTime_ = 0.0f;
	attackCount_ = 0;
	pBoss_->SetCameraRang(cameraRang_);
	pBoss_->SetCameraFollowSpeed(cameraFollowSpeed_);
	pBoss_->GetAnimModel()->ChangeAnimation("punch");
	pBoss_->GetAnimModel()->IsRoopAnimation(false);
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
			pBoss_->GetAnimModel()->IsRoopAnimation(true);
			AudioPlayer::GetInstance()->SoundPlayWave(*attackSE_, 0.35f);
			pBoss_->WaveWallAttack();
			attackCount_++;
		} else {
			pBoss_->GetAnimModel()->IsRoopAnimation(false);
			coolTime_ -= FPSKeeper::DeltaTimeFrame();
		}

		break;
		///---------------------------------------------------------------------------------------
		/// 通常へ移行
		///---------------------------------------------------------------------------------------
	case BossSwordAttack::Step::TOROOT:
		pBoss_->GetAnimModel()->IsRoopAnimation(true);
		pBoss_->ChangeBehavior(std::make_unique<BossRoot>(pBoss_));
		break;
	default:
		break;
	}

}

void BossSwordAttack::Debug() {
}
