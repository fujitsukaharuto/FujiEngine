
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


}

EnemyExplotion::~EnemyExplotion() {

}

void EnemyExplotion::Update() {
	
	// 死ぬ
	pBaseEnemy_->SetIsDeath(true);
	
}

void EnemyExplotion::Debug() {
	

}

