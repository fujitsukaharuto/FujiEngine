
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
	
	jumpSpeed_ = pBaseEnemy_->GetBoundPower();
	pBaseEnemy_->SetIsCollision(true);
}

EnemyBound::~EnemyBound() {

}

void  EnemyBound::Update() {
	
	pBaseEnemy_->Jump(jumpSpeed_);
	


}

void  EnemyBound::Debug() {
	

}

