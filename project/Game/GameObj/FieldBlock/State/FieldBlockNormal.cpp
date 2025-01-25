/// behavior
#include"FieldBlockNormal.h"
#include"FieldBlockDaungerous.h"
#include"GameObj/Player/Player.h"
#include"GameObj/FieldBlock/FieldBlock.h"
/// obj
#include"GameObj/Enemy/BaseEnemy.h"

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

	step_ = Step::NORMAL;
}

FieldBlockNormal::~FieldBlockNormal() {

}

void  FieldBlockNormal::Update() {
	switch (step_)
	{
	case FieldBlockNormal::Step::NORMAL:
		///---------------------------------------------------------
		/// 通常
		///----------------------------------------------------------

		if (pFieldBlock_->GetHp() > 0)break;
		step_ = Step::DAUNGEROUSMODE;

			break;
	case FieldBlockNormal::Step::DAUNGEROUSMODE:
		///---------------------------------------------------------
		/// 危険モード
		///----------------------------------------------------------
		pFieldBlock_->ChangeState(std::make_unique<FieldBlockDaungerous>(pFieldBlock_));
		break;
	default:
		break;
	}


}

void  FieldBlockNormal::Debug() {


}

