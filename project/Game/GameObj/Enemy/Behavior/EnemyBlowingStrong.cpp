/// behavior
#include"EnemyBlowingStrong.h"
#include"GameObj/Player/Player.h"
/// obj
#include"GameObj/Enemy/BaseEnemy.h"

/// math
#include"MathFunction.h"
#include"FPSKeeper.h"
#include "Particle/ParticleManager.h"

//初期化
EnemyBlowingStrong::EnemyBlowingStrong(BaseEnemy* boss)
	: BaseEnemyBehaivor("EnemyBlowingStrong", boss) {
	
	rotateSpeed_ = 30.0f;
	blowPower_ = pBaseEnemy_->GetParamater().blowingPower[static_cast<int>(BaseEnemy::BlowingPower::MaxPower)];
	direction_ = pBaseEnemy_->GetPlayer()->GetKikDirection();


	ParticleManager::Load(kickEmit1_, "hardKick1");
	ParticleManager::Load(kickEmit2_, "hardKick2");
	ParticleManager::Load(kickEmit3_, "hardKick3");
	ParticleManager::Load(kickEmit4_, "hardKick4");

	kickEmit1_.pos = pBaseEnemy_->GetModel()->GetWorldPos();
	kickEmit2_.pos = pBaseEnemy_->GetModel()->GetWorldPos();
	kickEmit3_.pos = pBaseEnemy_->GetModel()->GetWorldPos();
	kickEmit4_.pos = pBaseEnemy_->GetModel()->GetWorldPos();

	kickEmit1_.Burst();
	kickEmit2_.Burst();
	kickEmit3_.Burst();
	kickEmit4_.Burst();

}

EnemyBlowingStrong::~EnemyBlowingStrong() {

}

void  EnemyBlowingStrong::Update() {

	pBaseEnemy_->AddPosition(Vector3(
		blowPower_ * direction_.x * FPSKeeper::DeltaTimeRate() * pBaseEnemy_->GetBlowDirection(),
		blowPower_ * direction_.y * FPSKeeper::DeltaTimeRate(),
		0.0f));

	rotateZ_ += rotateSpeed_ * FPSKeeper::DeltaTimeRate();
	pBaseEnemy_->SetRotation(Vector3(0, 0, rotateZ_));
	
	
}

void  EnemyBlowingStrong::Debug() {
	

}

