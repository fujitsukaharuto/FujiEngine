
/// behavior
#include"EnemyRoot.h"

/// obj
#include"GameObj/Enemy/BaseEnemy.h"

/// math
#include"MathFunction.h"
#include"FPSKeeper.h"


//初期化
EnemyRoot::EnemyRoot(BaseEnemy* boss)
	: BaseEnemyBehaivor("EnemyRoot", boss) {
	
	step_ = Step::BOUND; /// ステップ初期化
}

EnemyRoot::~EnemyRoot() {

}

void EnemyRoot::Update() {
	
	
	
}

void EnemyRoot::Debug() {
	

}

