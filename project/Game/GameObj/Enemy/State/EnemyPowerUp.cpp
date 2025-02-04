
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
	
	ease_.time = 0.0f;
	ease_.maxTime = 0.4f;
	ease_.amplitude = 2.0f;
	ease_.period = 0.2f;

	stopTimeMax_ = 0.1f;

	FPSKeeper::SetTimeScale(0.1f);
	step_ = Step::SCALEUP; /// ステップ初期化

}

EnemyPowerUp::~EnemyPowerUp() {
	FPSKeeper::SetTimeScale(1.0f);
}

void EnemyPowerUp::Update() {

	stopTime_ += FPSKeeper::NormalDeltaTime();
	if (stopTime_ >= stopTimeMax_) {
		FPSKeeper::SetTimeScale(1.0f);
	}

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
		if (stopTime_ < stopTimeMax_) break;
		pBaseEnemy_->ChangeState(std::make_unique<EnemyNoneState>(pBaseEnemy_));
		break;
	default:
		break;
	}


}

void EnemyPowerUp::Debug() {


}

