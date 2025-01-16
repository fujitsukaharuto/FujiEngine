
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
	pBaseEnemy_->SetIsCollision(true);

	pBaseEnemy_->SetGravity(pBaseEnemy_->GetGravityForKik());
	pBaseEnemy_->SetFallMaxSpeed(pBaseEnemy_->GetFallMaxForKik());

	rotateSpeed_ = 30.0f;
}

EnemyJump::~EnemyJump() {

}

void  EnemyJump::Update() {
	rotateZ_ += rotateSpeed_ * FPSKeeper::NormalDeltaTime();
	pBaseEnemy_->SetRotation(Vector3(0, 0, rotateZ_));
	pBaseEnemy_->Jump(jumpSpeed_);
	
}

void  EnemyJump::Debug() {
	

}

