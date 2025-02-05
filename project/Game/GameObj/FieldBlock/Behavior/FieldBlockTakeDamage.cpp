/// behavior
#include"FieldBlockTakeDamage.h"
#include"FieldBlockNoDamage.h"

#include"GameObj/Player/Player.h"
#include"GameObj/FieldBlock/FieldBlock.h"
/// obj
#include"GameObj/Enemy/BaseEnemy.h"

/// math
#include"MathFunction.h"
#include "Math/Random.h"
#include"FPSKeeper.h"


//初期化
FieldBlockTakeDamage::FieldBlockTakeDamage(FieldBlock* boss)
	: BaseFieldBlockBehavior("FieldBlockTakeDamage", boss) {
	
	pFieldBlock_->GetModel()->SetColor(Vector4(1, 0, 0, 1));
	originPos_ = pFieldBlock_->GetModel()->transform.translate;
	collTime_ = 0.0f;
	pFieldBlock_->DecrementHp();

	//テクスチャ切り替え
	pFieldBlock_->ChangeTexture(pFieldBlock_->GetHp());
	
	//体力によってモデル切り替え
	step_ = Step::DAMAGE;
}

FieldBlockTakeDamage::~FieldBlockTakeDamage() {

}

void  FieldBlockTakeDamage::Update() {
	float limitte = 0;
	float shakeRate = 0;
	float shakeValue = 0;
	switch (step_)
	{
	case FieldBlockTakeDamage::Step::DAMAGE:
		///---------------------------------------------------------
		/// ダメージ
		///----------------------------------------------------------
		collTime_ += FPSKeeper::NormalDeltaTime();

		// シェイク
		limitte = pFieldBlock_->GetParamater().damageCollTime_;
		shakeRate = limitte - collTime_ / limitte;
		shakeValue = 1.0f * shakeRate;
		pFieldBlock_->GetModel()->transform.translate = originPos_ + Random::GetVector3({ -shakeValue,shakeValue }, { -shakeValue,shakeValue }, { -shakeValue,shakeValue });

		if (collTime_ < limitte)break;
		step_ = Step::NODAMAGE;
		break;
	case FieldBlockTakeDamage::Step::NODAMAGE:
		///---------------------------------------------------------
		/// 通常モードに
		///----------------------------------------------------------

		pFieldBlock_->GetModel()->transform.translate = originPos_;
		pFieldBlock_->ChangeBehavior(std::make_unique<FieldBlockNoDamage>(pFieldBlock_));
		break;
	default:
		break;
	}
}

void  FieldBlockTakeDamage::Debug() {
	

}

