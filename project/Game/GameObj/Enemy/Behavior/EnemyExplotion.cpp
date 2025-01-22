
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

	pBaseEnemy_->SetGravity(pBaseEnemy_->GetBoundGravity());
	pBaseEnemy_->SetFallMaxSpeed(pBaseEnemy_->GetFallMaxForBound());

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
		pBaseEnemy_->Jump(jumpSpeed_);

		if (pBaseEnemy_->GetTrans().translate.y > BaseEnemy::InitY_)break;
		pBaseEnemy_->DecrementDeathCount(); 
		step_ = Step::REBOUND;
		
		break;

	case EnemyBound::Step::REBOUND:
		///-------------------------------------------------------------
        ///　再バウンド
        ///-------------------------------------------------------------- 
		pBaseEnemy_->ChangeBehavior(std::make_unique<EnemyBound>(pBaseEnemy_));

		break;
	default:
		break;
	}
	
}

void  EnemyBound::Debug() {
	

}

