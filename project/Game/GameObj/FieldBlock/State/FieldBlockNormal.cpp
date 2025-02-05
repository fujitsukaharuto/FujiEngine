/// behavior
#include"FieldBlockNormal.h"
#include"FieldBlockDaungerous.h"
#include"GameObj/Player/Player.h"
#include"GameObj/FieldBlock/FieldBlock.h"
/// obj
#include"GameObj/Enemy/BaseEnemy.h"
#include"Particle/ParticleManager.h"
/// math
#include"MathFunction.h"
#include"FPSKeeper.h"


//初期化
FieldBlockNormal::FieldBlockNormal(FieldBlock* boss)
	: BaseFieldBlockState("FieldBlockNormal", boss) {
	// parm
	pFieldBlock_->ChangeModel(static_cast<size_t>(FieldBlock::Tag::NORMAL));
	pFieldBlock_->SetHp(pFieldBlock_->GetParamater().breakCount);
	pFieldBlock_->SetTag(static_cast<size_t>(FieldBlock::Tag::NORMAL));
	pFieldBlock_->ChangeTexture(pFieldBlock_->GetHp());

	/// particle
	
	ParticleManager::Load(ChangeEmit_, "groundBreak");
	ChangeEmit_.pos = pFieldBlock_->GetModel()->GetWorldPos();
	Vector3 emitSizeMax = pFieldBlock_->GetCollisionSize() * 0.5f;
	Vector3 emitSizeMin = pFieldBlock_->GetCollisionSize() * -0.5f;
	ChangeEmit_.emitSizeMax = { emitSizeMax.x,8.0f,emitSizeMin.z - 2.0f };
	ChangeEmit_.emitSizeMin = { emitSizeMin.x,2.0f,emitSizeMin.z - 3.0f };

	kwaitTime_ = 0.4f;
	step_ = Step::NORMAL;

	
}

FieldBlockNormal::~FieldBlockNormal() {

}

void  FieldBlockNormal::Update() {
	switch (step_)
	{
		///---------------------------------------------------------
		/// 通常
		///----------------------------------------------------------

	case FieldBlockNormal::Step::NORMAL:
		
		/// HP0で破壊モードへ
		if (pFieldBlock_->GetHp() > 0)break;	
		ChangeEmit_.Burst();
		step_ = Step::WAIT;

			break;
		///---------------------------------------------------------
		/// 待機モード
		///----------------------------------------------------------
	case FieldBlockNormal::Step::WAIT:
		waitTime_ += FPSKeeper::DeltaTimeRate();
		if (waitTime_ < kwaitTime_)break;
		step_ = Step::DAUNGEROUSMODE;
		break;
		///---------------------------------------------------------
		/// 危険モード
		///----------------------------------------------------------
	case FieldBlockNormal::Step::DAUNGEROUSMODE:
		
		pFieldBlock_->ChangeState(std::make_unique<FieldBlockDaungerous>(pFieldBlock_));
		break;
	default:
		break;
	}


}

void  FieldBlockNormal::Debug() {


}

