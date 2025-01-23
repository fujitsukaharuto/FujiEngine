
/// behavior
#include"EnemySpawnFall.h"

/// obj
#include"GameObj/Enemy/BaseEnemy.h"

/// math
#include"MathFunction.h"
#include"FPSKeeper.h"


//初期化
EnemySpawnFall::EnemySpawnFall(BaseEnemy* boss)
	: BaseEnemyBehaivor("EnemySpawnFall", boss) {

	speed_ = pBaseEnemy_->GetParamater().spawnFallSpeed;
	boundSpeed_ = pBaseEnemy_->GetParamater().spawnBoundSpeed;
	rotateSpeed_ = 30.0f;

	step_ = Step::FALL;
}

EnemySpawnFall::~EnemySpawnFall() {

}

void EnemySpawnFall::Update() {
	switch (step_)
	{
	case EnemySpawnFall::Step::FALL:
	///----------------------------------------------------------------------
	/// 落ちる 
	///----------------------------------------------------------------------
		// Y軸方向に落下
		pBaseEnemy_->AddPosition(Vector3(0, -speed_ * FPSKeeper::DeltaTimeRate(), 0));

		if (pBaseEnemy_->GetModel()->GetWorldPos().y > BaseEnemy::BoundPosY_)break;
		step_ = Step::BOUND;

		break;
	case EnemySpawnFall::Step::BOUND:
		///----------------------------------------------------------------------
	   /// バウンド 
	  ///------------------------------------------------------------------------
		rotateX_ += rotateSpeed_ * FPSKeeper::DeltaTimeRate();
		pBaseEnemy_->SetRotationX(rotateX_);
		pBaseEnemy_->Bound(boundSpeed_);
		
		break;
	default:
		break;
	}
	
	



}

void EnemySpawnFall::Debug() {


}

