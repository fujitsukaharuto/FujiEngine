
/// behavior
#include"EnemyNoneState.h"

/// obj
#include"GameObj/Enemy/BaseEnemy.h"

/// math
#include"MathFunction.h"
#include"FPSKeeper.h"


//初期化
EnemyNoneState::EnemyNoneState(BaseEnemy* boss)
	: BaseEnemyState("EnemyNoneState", boss) {
	
	step_ = Step::BOUND; /// ステップ初期化
}

EnemyNoneState::~EnemyNoneState() {

}

void EnemyNoneState::Update() {
	
	
	
}

void EnemyNoneState::Debug() {
	

}

