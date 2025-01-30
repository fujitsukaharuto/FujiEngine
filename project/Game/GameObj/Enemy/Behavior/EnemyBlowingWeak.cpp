/// behavior
#include"EnemyBlowingWeak.h"
#include"EnemyExplotion.h"

#include"GameObj/Player/Player.h"
/// obj
#include"GameObj/Enemy/BaseEnemy.h"

/// math
#include "Particle/ParticleManager.h"
#include"MathFunction.h"
#include"FPSKeeper.h"


//初期化
EnemyBlowingWeak::EnemyBlowingWeak(BaseEnemy* boss)
	: BaseEnemyBehaivor("EnemyBlowingWeak", boss) {

	rotateSpeed_ = 30.0f;
	// 力と向き
	blowPower_ = pBaseEnemy_->GetParamater().blowingPower[static_cast<int>(BaseEnemy::BlowingPower::WEAK)];
	direction_ = pBaseEnemy_->GetPlayer()->GetKikDirection();

	// 爆発時間
	explotionTime_ = 0.0f;

	step_ = Step::BLOW;

	/// emitter
	ParticleManager::Load(kickEmit1_, "kick1");
	ParticleManager::Load(kickEmit2_, "kick2");
	ParticleManager::Load(kickEmit3_, "kick3");
	ParticleManager::Load(kickEmit4_, "kick4");


	kickEmit1_.pos = pBaseEnemy_->GetModel()->GetWorldPos();
	kickEmit2_.pos = pBaseEnemy_->GetModel()->GetWorldPos();
	kickEmit3_.pos = pBaseEnemy_->GetModel()->GetWorldPos();
	kickEmit4_.pos = pBaseEnemy_->GetModel()->GetWorldPos();

	kickEmit1_.Burst();
	kickEmit2_.Burst();
	kickEmit3_.Burst();
	kickEmit4_.Burst();

	ParticleManager::Load(flyEmit1_, "flyEnemy1");
	ParticleManager::Load(flyEmit2_, "flyEnemy2");

	flyEmit1_.frequencyTime = 2.0f;
	flyEmit1_.count = 1;
	flyEmit1_.grain.lifeTime_ = 6.0f;

	flyEmit2_.frequencyTime = 2.0f;
	flyEmit2_.count = 1;
	flyEmit2_.grain.lifeTime_ = 6.0f;

	kickHit_ = Audio::GetInstance()->SoundLoadWave("kickHit.wav");
	Audio::GetInstance()->SoundPlayWave(kickHit_);

}

EnemyBlowingWeak::~EnemyBlowingWeak() {

}

void  EnemyBlowingWeak::Update() {

	Vector3 flydirection{};

	switch (step_)
	{
	case EnemyBlowingWeak::Step::BLOW:
		///---------------------------------------------------------
		/// ぶっ飛び中
		///----------------------------------------------------------

		explotionTime_ += FPSKeeper::DeltaTimeRate();

		pBaseEnemy_->AddPosition(Vector3(
			blowPower_ * direction_.x * FPSKeeper::DeltaTimeRate() * pBaseEnemy_->GetBlowDirection(),
			blowPower_ * direction_.y * FPSKeeper::DeltaTimeRate(),
			0.0f));
		rotateZ_ += rotateSpeed_ * FPSKeeper::DeltaTimeRate();
		pBaseEnemy_->SetRotation(Vector3(0, 0, rotateZ_));

		// particle
		flydirection = { direction_.x * FPSKeeper::DeltaTimeRate() * pBaseEnemy_->GetBlowDirection(), direction_.y * FPSKeeper::DeltaTimeRate() ,0.0f };
		flydirection = flydirection * 25.0f;
		flyEmit1_.pos = pBaseEnemy_->GetModel()->GetWorldPos();
		flyEmit2_.pos = pBaseEnemy_->GetModel()->GetWorldPos();
		flyEmit1_.RandomSpeed({ -flydirection.x,-flydirection.x }, { -flydirection.y,-flydirection.y }, { 0.0f,0.0f });
		flyEmit2_.RandomSpeed({ -flydirection.x,-flydirection.x }, { -flydirection.y,-flydirection.y }, { 0.0f,0.0f });
		flyEmit1_.Emit();
		flyEmit2_.Emit();

		if (explotionTime_ < pBaseEnemy_->GetExplotionTime())break;
		step_ = Step::EXPLOTION;
		break;
	case EnemyBlowingWeak::Step::EXPLOTION:
		///---------------------------------------------------------
		/// 死ぬ
		///----------------------------------------------------------
		pBaseEnemy_->ChangeBehavior(std::make_unique<EnemyExplotion>(pBaseEnemy_));
		break;
	default:
		break;
	}

	

}

void  EnemyBlowingWeak::Debug() {


}

