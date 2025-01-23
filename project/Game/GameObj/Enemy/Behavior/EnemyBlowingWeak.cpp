/// behavior
#include"EnemyBlowingWeak.h"
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

	pBaseEnemy_->AddPosition(blowPower_ * direction_ * FPSKeeper::DeltaTimeRate());

	rotateZ_ += rotateSpeed_ * FPSKeeper::DeltaTimeRate();
	pBaseEnemy_->SetRotation(Vector3(0, 0, rotateZ_));


}

void  EnemyBlowingWeak::Debug() {


}

