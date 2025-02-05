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
#include "Particle/ParticleManager.h"

//初期化
FieldBlockTakeDamage::FieldBlockTakeDamage(FieldBlock* boss)
	: BaseFieldBlockBehavior("FieldBlockTakeDamage", boss) {
	
	pFieldBlock_->GetModel()->SetColor(Vector4(1, 0, 0, 1));
	originPos_ = pFieldBlock_->GetModel()->transform.translate;
	collTime_ = 0.0f;
	pFieldBlock_->DecrementHp();

	//テクスチャ切り替え
	pFieldBlock_->ChangeTexture(pFieldBlock_->GetHp());
	
	ParticleManager::Load(emit_, "groundCrack1");
	emit_.pos = originPos_;
	emit_.pos.y += 1.0f;
	Vector3 emitSizeMax = pFieldBlock_->GetCollisionSize() * 0.5f;
	Vector3 emitSizeMin = pFieldBlock_->GetCollisionSize() * -0.5f;
	emit_.emitSizeMax = { emitSizeMax.x,1.0f,emitSizeMax.z };
	emit_.emitSizeMin.x = emitSizeMin.x;
	emit_.emitSizeMin.z = emitSizeMin.z;
	emit_.Burst();

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

