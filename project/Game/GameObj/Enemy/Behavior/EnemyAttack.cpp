
/// behavior
#include"EnemyAttack.h"

/// obj
#include"GameObj/Enemy/BaseEnemy.h"

/// math
#include"MathFunction.h"
#include"FPSKeeper.h"


//初期化
EnemyAttack::EnemyAttack(BaseEnemy* boss)
	: BaseEnemyBehaivor("EnemyAttack", boss) {
	
	step_ = Step::BOUND; /// ステップ初期化
}

EnemyAttack::~EnemyAttack() {

}

void EnemyAttack::Update() {
	
	
	
}

void EnemyAttack::Debug() {
	

}

