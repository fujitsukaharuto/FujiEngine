/// behavior
#include"LifeUIDeath.h"
#include"LifeUIRoot.h"

/// obj
#include"GameObj/LifeUI/LifeUI.h"
/// frame
#include"DX/FPSKeeper.h"
/// inupt

#include<imgui.h>

//初期化
LifeUIDeath::LifeUIDeath(LifeUI* boss)
	: BaseLifeUIBehavior("LifeUIDeath", boss) {

	/// ===================================================
	/// 変数初期化
	/// ===================================================
	step_ = Step::WAIT;

}

LifeUIDeath ::~LifeUIDeath() {

}

//更新
void LifeUIDeath::Update() {

	switch (step_)
	{
		///-------------------------------------------------------
		/// 待機
		///-------------------------------------------------------
	case LifeUIDeath::Step::WAIT:

		waitTime_ += FPSKeeper::NormalDeltaTime();//deltatime加算

		// 次のステップ
		if (waitTime_ < pLifeUI_->GetParamater().moveWaitTime_)break;
		step_ = Step::UIMOVE;
		break;
		///-------------------------------------------------------
		/// UI移動
		///-------------------------------------------------------
	case LifeUIDeath::Step::UIMOVE:
		deathMoveTime_ += FPSKeeper::NormalDeltaTime();
		
		// イージング適応
		pLifeUI_->SetPos       (EaseInCirc(pLifeUI_->GetParamater().basePosNormal, pLifeUI_->GetParamater().basePosDeath, deathMoveTime_, pLifeUI_->GetParamater().moveUIEaseTime_));
		pLifeUI_->SetSizeWidth (EaseInCirc(pLifeUI_->GetParamater().kTextureWidth, pLifeUI_->GetParamater().deathTextureWidth_, deathMoveTime_, pLifeUI_->GetParamater().moveUIEaseTime_));
		pLifeUI_->SetSizeHeigth(EaseInCirc(pLifeUI_->GetParamater().kTextureHeight, pLifeUI_->GetParamater().deathTextureHeigth_, deathMoveTime_, pLifeUI_->GetParamater().moveUIEaseTime_));
		pLifeUI_->SetOffSet    (EaseInCirc(pLifeUI_->GetParamater().offsetNormal, pLifeUI_->GetParamater().offsetDeath, deathMoveTime_, pLifeUI_->GetParamater().moveUIEaseTime_));


		// 次の
		if(deathMoveTime_ < pLifeUI_->GetParamater().moveUIEaseTime_)break;
		deathMoveTime_ = pLifeUI_->GetParamater().moveUIEaseTime_;
		pLifeUI_->SetPos(pLifeUI_->GetParamater().basePosDeath);
		step_ = Step::BREAK;
		break;
		///-------------------------------------------------------
		/// ライフUI破壊
		///-------------------------------------------------------
	case LifeUIDeath::Step::BREAK:
		pLifeUI_->LifeBreak();
		step_ = Step::RETURN;
		break;
		///-------------------------------------------------------
		/// UI元の位置に戻る
		///-------------------------------------------------------
	case LifeUIDeath::Step::RETURN:
		returnMoveTime_ += FPSKeeper::NormalDeltaTime();

		pLifeUI_->SetPos       (EaseInCirc(pLifeUI_->GetParamater().basePosDeath, pLifeUI_->GetParamater().basePosNormal, returnMoveTime_, pLifeUI_->GetParamater().moveUIEaseTime_));
		pLifeUI_->SetSizeWidth (EaseInCirc(pLifeUI_->GetParamater().deathTextureWidth_, pLifeUI_->GetParamater().kTextureWidth,  deathMoveTime_, pLifeUI_->GetParamater().moveUIEaseTime_));
		pLifeUI_->SetSizeHeigth(EaseInCirc(pLifeUI_->GetParamater().deathTextureHeigth_, pLifeUI_->GetParamater().kTextureHeight, deathMoveTime_, pLifeUI_->GetParamater().moveUIEaseTime_));
		pLifeUI_->SetOffSet    (EaseInCirc(pLifeUI_->GetParamater().offsetDeath, pLifeUI_->GetParamater().offsetNormal,  deathMoveTime_, pLifeUI_->GetParamater().moveUIEaseTime_));


		// 次の
		if (returnMoveTime_ < pLifeUI_->GetParamater().moveUIEaseTime_)break;
		deathMoveTime_ = pLifeUI_->GetParamater().moveUIEaseTime_;
		pLifeUI_->SetPos(pLifeUI_->GetParamater().basePosNormal);
		step_ = Step::GOROOT;
		break;
		///-------------------------------------------------------
		/// 通常振る舞いに戻る
		///-------------------------------------------------------
	case LifeUIDeath::Step::GOROOT:
		pLifeUI_->ChangeBehavior(std::make_unique<LifeUIRoot>(pLifeUI_));
		break;
	default:
		break;
	}

}

void  LifeUIDeath::Debug() {

}

