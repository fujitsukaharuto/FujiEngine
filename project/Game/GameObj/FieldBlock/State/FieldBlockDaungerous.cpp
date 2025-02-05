/// behavior
#include"FieldBlockDaungerous.h"
#include"FieldBlockNormal.h"
#include"GameObj/Player/Player.h"
#include"GameObj/FieldBlock/FieldBlock.h"
/// obj
#include"GameObj/Enemy/BaseEnemy.h"

/// math
#include"MathFunction.h"
#include"FPSKeeper.h"
#include "Particle/ParticleManager.h"

//初期化
FieldBlockDaungerous::FieldBlockDaungerous(FieldBlock* boss)
	: BaseFieldBlockState("FieldBlockDaungerous", boss) {
	
	pFieldBlock_->ChangeModel(static_cast<size_t>(FieldBlock::Tag::DAUNGEROUS));
	pFieldBlock_->ChangeTexture(pFieldBlock_->GetHp());
	pFieldBlock_->GetModel()->SetColor(Vector4(1, 1, 0, 1));
	pFieldBlock_->SetTag(static_cast<size_t>(FieldBlock::Tag::DAUNGEROUS));
	restoreTime_ = 0.0f;

	ParticleManager::Load(darkEmit_, "dark");
	darkEmit_.pos = pFieldBlock_->GetModel()->GetWorldPos();
	Vector3 emitSizeMax = pFieldBlock_->GetCollisionSize() * 0.5f;
	Vector3 emitSizeMin = pFieldBlock_->GetCollisionSize() * -0.5f;
	darkEmit_.emitSizeMax = emitSizeMax;
	darkEmit_.emitSizeMin.x = emitSizeMin.x;
	darkEmit_.emitSizeMin.z = emitSizeMin.z;

	step_ = Step::DAUNGEROUSMODE;

	
}

FieldBlockDaungerous::~FieldBlockDaungerous() {

}

void  FieldBlockDaungerous::Update() {

	switch (step_)
	{
	case FieldBlockDaungerous::Step::DAUNGEROUSMODE:
		///---------------------------------------------------------
		/// 危険モード
		///----------------------------------------------------------
		restoreTime_ += FPSKeeper::DeltaTimeRate();
		darkEmit_.Emit();
		if (restoreTime_<pFieldBlock_->GetParamater().restoreTime_)break;
		step_ = Step::GONORMAL;

		break;
	case FieldBlockDaungerous::Step::GONORMAL:
		///---------------------------------------------------------
		/// 通常
		///----------------------------------------------------------
		pFieldBlock_->Reset();
		break;
	default:
		break;
	}
}

void  FieldBlockDaungerous::Debug() {
	

}

