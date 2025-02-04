
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

	//パーティクルと音
	pBaseEnemy_->ExplotionRendition();

	step_ = Step::WAIT;

	waitTimeMax_ = 0.5f;
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

