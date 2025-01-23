
/// behavior
#include"EnemyExplotion.h"

/// obj
#include"GameObj/Enemy/BaseEnemy.h"

/// math
#include"MathFunction.h"
#include"FPSKeeper.h"


//初期化
EnemyExplotion::EnemyExplotion(BaseEnemy* boss)
	: BaseEnemyBehaivor("EnemyExplotion", boss) {
	

}

EnemyExplotion::~EnemyExplotion() {

}

void EnemyExplotion::Update() {
	
	pBaseEnemy_->SetIsDeath(true);
	
}

void EnemyExplotion::Debug() {
	

}

