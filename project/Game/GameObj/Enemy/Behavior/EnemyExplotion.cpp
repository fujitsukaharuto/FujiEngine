
/// behavior
#include"EnemyExplotion.h"

/// obj
#include"GameObj/Enemy/BaseEnemy.h"

/// math
#include "Particle/ParticleManager.h"
#include"MathFunction.h"
#include"FPSKeeper.h"


//初期化
EnemyExplotion::EnemyExplotion(BaseEnemy* boss)
	: BaseEnemyBehaivor("EnemyExplotion", boss) {


	/// emitter
	ParticleManager::Load(bombEmit1_, "bomb1");
	ParticleManager::Load(bombEmit2_, "bomb2");
	ParticleManager::Load(bombEmit3_, "bomb3");

	bombEmit1_.pos = pBaseEnemy_->GetModel()->GetWorldPos();
	bombEmit2_.pos = pBaseEnemy_->GetModel()->GetWorldPos();
	bombEmit3_.pos = pBaseEnemy_->GetModel()->GetWorldPos();

	bombEmit1_.Burst();
	bombEmit2_.Burst();
	bombEmit3_.Burst();

	step_ = Step::WAIT;

	waitTimeMax_ = 0.1f;
	waitTime_ = waitTimeMax_;
}

EnemyExplotion::~EnemyExplotion() {

}

void EnemyExplotion::Update() {
	switch (step_)
	{
	case EnemyExplotion::Step::WAIT:
		///-----------------------------------------------------------------
		///
		///-----------------------------------------------------------------
		waitTime_ += FPSKeeper::NormalDeltaTime();
		if (waitTime_ < waitTimeMax_)break;
		step_ = Step::DEATH;
		break;
	case EnemyExplotion::Step::DEATH:
		///-----------------------------------------------------------------
		///
		///-----------------------------------------------------------------
		// 死ぬ
		pBaseEnemy_->SetIsDeath(true);
		break;
	default:
		break;
	}
	
	
}

void EnemyExplotion::Debug() {
	

}

