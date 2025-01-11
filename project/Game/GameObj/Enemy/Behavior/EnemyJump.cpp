
/// behavior
#include"EnemyJump.h"

/// obj
#include"GameObj/Enemy/BaseEnemy.h"

/// math
#include"MathFunction.h"
#include"FPSKeeper.h"


//初期化
EnemyJump::EnemyJump(BaseEnemy* boss)
	: BaseEnemyBehaivor("EnemyJump", boss) {
	
	step_ = Step::BOUND; /// ステップ初期化
	jumpSpeed_ = pBaseEnemy_->GetJumpSpeed(0);
}

EnemyJump::~EnemyJump() {

}

void  EnemyJump::Update() {
	
	pBaseEnemy_->Jump(jumpSpeed_);
	
}

void  EnemyJump::Debug() {
	

}

