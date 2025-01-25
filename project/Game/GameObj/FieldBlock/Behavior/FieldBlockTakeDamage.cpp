/// behavior
#include"FieldBlockTakeDamage.h"
#include"FieldBlockNoDamage.h"

#include"GameObj/Player/Player.h"
#include"GameObj/FieldBlock/FieldBlock.h"
/// obj
#include"GameObj/Enemy/BaseEnemy.h"

/// math
#include"MathFunction.h"
#include"FPSKeeper.h"


//初期化
FieldBlockTakeDamage::FieldBlockTakeDamage(FieldBlock* boss)
	: BaseFieldBlockBehavior("FieldBlockTakeDamage", boss) {
	
	pFieldBlock_->GetModel()->SetColor(Vector4(1, 0, 0, 1));
	collTime_ = 0.0f;
	pFieldBlock_->DecrementHp();
	step_ = Step::DAMAGE;
}

FieldBlockTakeDamage::~FieldBlockTakeDamage() {

}

void  FieldBlockTakeDamage::Update() {
	switch (step_)
	{
	case FieldBlockTakeDamage::Step::DAMAGE:
		///---------------------------------------------------------
		/// ダメージ
		///----------------------------------------------------------
		collTime_ += FPSKeeper::NormalDeltaTime();
		if (collTime_ < pFieldBlock_->GetParamater().damageCollTime_)break;
		step_ = Step::NODAMAGE;
		break;
	case FieldBlockTakeDamage::Step::NODAMAGE:
		///---------------------------------------------------------
		/// 通常モードに
		///----------------------------------------------------------

		pFieldBlock_->ChangeBehavior(std::make_unique<FieldBlockNoDamage>(pFieldBlock_));
		break;
	default:
		break;
	}
}

void  FieldBlockTakeDamage::Debug() {
	

}

