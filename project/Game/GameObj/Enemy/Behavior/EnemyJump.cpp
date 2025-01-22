
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
	
	

	rotateSpeed_ = 30.0f;
}

EnemyJump::~EnemyJump() {

}

void  EnemyJump::Update() {
	rotateZ_ += rotateSpeed_ * FPSKeeper::DeltaTimeRate();
	pBaseEnemy_->SetRotation(Vector3(0, 0, rotateZ_));
	
	
}

void  EnemyJump::Debug() {
	

}

