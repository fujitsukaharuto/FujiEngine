
/// behavior
#include"EnemyPowerUp.h"
#include"EnemyNoneState.h"

/// obj
#include"GameObj/Enemy/BaseEnemy.h"

/// math
#include"MathFunction.h"
#include"FPSKeeper.h"


//初期化
EnemyPowerUp::EnemyPowerUp(BaseEnemy* boss)
	: BaseEnemyState("EnemyPowerUp", boss) {

	ease_.maxTime = 0.4f;
	ease_.amplitude = 3.8f;
	ease_.period = 0.2f;
	/*saveScale_ = pBaseEnemy_->GetPowerUpScale();*/
	step_ = Step::SCALEUP; /// ステップ初期化
}

EnemyPowerUp::~EnemyPowerUp() {

}

void EnemyPowerUp::Update() {

	switch (step_)
	{
	case EnemyPowerUp::Step::SCALEUP:
		///---------------------------------------------------------------------------------------
		/// スケールアップ
		///---------------------------------------------------------------------------------------
		ease_.time += FPSKeeper::DeltaTimeRate();
		pBaseEnemy_->SetScale(EaseAmplitudeScale(pBaseEnemy_->GetPowerUpScale(), ease_.time, ease_.maxTime, ease_.amplitude, ease_.period));

		/// タイム超越で
		if (ease_.time < ease_.maxTime)break;

		ease_.time = ease_.maxTime;
		pBaseEnemy_->SetScale(pBaseEnemy_->GetPowerUpScale());
		step_ = Step::RETUNROOT;

		break;
	case EnemyPowerUp::Step::RETUNROOT:
		///---------------------------------------------------------------------------------------
		/// 通常に戻る
		///---------------------------------------------------------------------------------------
		pBaseEnemy_->ChangeState(std::make_unique<EnemyNoneState>(pBaseEnemy_));
		break;
	default:
		break;
	}


}

void EnemyPowerUp::Debug() {


}

