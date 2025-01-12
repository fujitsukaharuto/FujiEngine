
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
	
	jumpSpeed_ = pBaseEnemy_->GetJumpSpeed(pBaseEnemy_->GetJumpPower());
	
}

EnemyJump::~EnemyJump() {

}

void  EnemyJump::Update() {
	
	pBaseEnemy_->Jump(jumpSpeed_);
	
}

void  EnemyJump::Debug() {
	

}

