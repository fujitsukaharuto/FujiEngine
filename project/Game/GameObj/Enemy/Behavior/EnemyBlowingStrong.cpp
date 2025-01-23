/// behavior
#include"EnemyBlowingStrong.h"
#include"GameObj/Player/Player.h"
/// obj
#include"GameObj/Enemy/BaseEnemy.h"

/// math
#include"MathFunction.h"
#include"FPSKeeper.h"


//初期化
EnemyBlowingStrong::EnemyBlowingStrong(BaseEnemy* boss)
	: BaseEnemyBehaivor("EnemyBlowingStrong", boss) {
	
	rotateSpeed_ = 30.0f;
	blowPower_ = pBaseEnemy_->GetParamater().blowingPower[static_cast<int>(BaseEnemy::BlowingPower::MaxPower)];
	direction_ = pBaseEnemy_->GetPlayer()->GetKikDirection();
}

EnemyBlowingStrong::~EnemyBlowingStrong() {

}

void  EnemyBlowingStrong::Update() {

	pBaseEnemy_->AddPosition(Vector3(
		blowPower_ * direction_.x * FPSKeeper::DeltaTimeRate() * pBaseEnemy_->GetBlowDirection(),
		blowPower_ * direction_.y * FPSKeeper::DeltaTimeRate(),
		0.0f));

	rotateZ_ += rotateSpeed_ * FPSKeeper::DeltaTimeRate();
	pBaseEnemy_->SetRotation(Vector3(0, 0, rotateZ_));
	
	
}

void  EnemyBlowingStrong::Debug() {
	

}

