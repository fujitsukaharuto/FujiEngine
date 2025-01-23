/// behavior
#include"EnemyBlowingWeak.h"
#include"EnemyExplotion.h"

#include"GameObj/Player/Player.h"
/// obj
#include"GameObj/Enemy/BaseEnemy.h"

/// math
#include"MathFunction.h"
#include"FPSKeeper.h"


//初期化
EnemyBlowingWeak::EnemyBlowingWeak(BaseEnemy* boss)
	: BaseEnemyBehaivor("EnemyBlowingWeak", boss) {

	rotateSpeed_ = 30.0f;
	// 力と向き
	blowPower_ = pBaseEnemy_->GetParamater().blowingPower[static_cast<int>(BaseEnemy::BlowingPower::WEAK)];
	direction_ = pBaseEnemy_->GetPlayer()->GetKikDirection();

	// 爆発時間
	explotionTime_ = 0.0f;

	step_ = Step::BLOW;

	/// emitter
	kickEmit1_.name = "kick1";
	kickEmit2_.name = "kick2";
	kickEmit3_.name = "kick3";
	kickEmit4_.name = "kick4";

	kickEmit1_.Load("kick1");
	kickEmit2_.Load("kick2");
	kickEmit3_.Load("kick3");
	kickEmit4_.Load("kick4");

}

EnemyBlowingWeak::~EnemyBlowingWeak() {

}

void  EnemyBlowingWeak::Update() {

	switch (step_)
	{
	case EnemyBlowingWeak::Step::BLOW:
		///---------------------------------------------------------
		/// ぶっ飛び中
		///----------------------------------------------------------

		explotionTime_ += FPSKeeper::DeltaTimeRate();

		pBaseEnemy_->AddPosition(Vector3(
			blowPower_ * direction_.x * FPSKeeper::DeltaTimeRate() * pBaseEnemy_->GetBlowDirection(),
			blowPower_ * direction_.y * FPSKeeper::DeltaTimeRate(),
			0.0f));
		rotateZ_ += rotateSpeed_ * FPSKeeper::DeltaTimeRate();
		pBaseEnemy_->SetRotation(Vector3(0, 0, rotateZ_));

		if (explotionTime_ < pBaseEnemy_->GetExplotionTime())break;
		step_ = Step::EXPLOTION;
		break;
	case EnemyBlowingWeak::Step::EXPLOTION:
		///---------------------------------------------------------
		/// 死ぬ
		///----------------------------------------------------------
		pBaseEnemy_->ChangeBehavior(std::make_unique<EnemyExplotion>(pBaseEnemy_));
		break;
	default:
		break;
	}

	

}

void  EnemyBlowingWeak::Debug() {


}

