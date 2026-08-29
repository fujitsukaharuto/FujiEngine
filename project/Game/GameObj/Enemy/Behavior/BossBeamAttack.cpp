#include "BossBeamAttack.h"

#include "Game/GameObj/Enemy/Boss.h"
#include "BossRoot.h"

using namespace Graphics;


BossBeamAttack::BossBeamAttack(Boss* pBoss) : BaseBossBehavior(pBoss) {
	step_ = Step::CHARGE;
	cameraRange_ = -45.0f;
	owner_->SetCameraRange(cameraRange_);
	owner_->SetCameraFollowSpeed(cameraFollowSpeed_);
	owner_->InitBeam();
	owner_->GetAnimeModel()->ChangeAnimation("BeamChargePose");
	owner_->ChainCount();
}

BossBeamAttack::~BossBeamAttack() {
}

void BossBeamAttack::Update() {

	switch (step_) {
		///---------------------------------------------------------------------------------------
		/// チャージ
		///---------------------------------------------------------------------------------------
	case BossBeamAttack::Step::CHARGE:

		if (owner_->BeamCharge()) {
			owner_->BeamChargeComplete();
			step_ = Step::ATTACK;
		}

		break;
		///---------------------------------------------------------------------------------------
		/// 攻撃
		///---------------------------------------------------------------------------------------
	case BossBeamAttack::Step::ATTACK:

		if (owner_->BeamAttack()) {
			step_ = Step::TOROOT;
		}

		break;
		///---------------------------------------------------------------------------------------
		/// 通常へ移行
		///---------------------------------------------------------------------------------------
	case BossBeamAttack::Step::TOROOT:
		owner_->ChangeBehavior(std::make_unique<BossRoot>(owner_));
		break;
	default:
		break;
	}

}
