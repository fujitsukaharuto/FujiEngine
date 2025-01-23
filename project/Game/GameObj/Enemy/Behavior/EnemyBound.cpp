
/// behavior
#include"EnemyBound.h"

/// obj
#include"GameObj/Enemy/BaseEnemy.h"

/// math
#include"MathFunction.h"
#include"FPSKeeper.h"


//初期化
EnemyBound::EnemyBound(BaseEnemy* boss)
	: BaseEnemyBehaivor("EnemyBound", boss) {
	

	step_ = Step::BOUND;
}

EnemyBound::~EnemyBound() {

}

void  EnemyBound::Update() {
	switch (step_)
	{
	case EnemyBound::Step::BOUND:
		///-------------------------------------------------------------
       ///　バウンド
       ///-------------------------------------------------------------- 
	   
		
		break;

	case EnemyBound::Step::REBOUND:
		///-------------------------------------------------------------
        ///　再バウンド
        ///-------------------------------------------------------------- 
	

		break;
	default:
		break;
	}
	
}

void  EnemyBound::Debug() {
	

}

