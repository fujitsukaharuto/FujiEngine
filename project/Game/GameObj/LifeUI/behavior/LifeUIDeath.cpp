/// behavior
#include"LifeUIDeath.h"
#include"LifeUIRoot.h"

/// obj
#include"GameObj/LifeUI/LifeUI.h"
/// frame
#include"DX/FPSKeeper.h"
/// inupt
#include "Particle/ParticleManager.h"

#include<imgui.h>

//初期化
LifeUIDeath::LifeUIDeath(LifeUI* boss)
	: BaseLifeUIBehavior("LifeUIDeath", boss) {

	/// ===================================================
	/// 変数初期化
	/// ===================================================
	
	// startParm
	startPram_.basePos = pLifeUI_->GetParamater().basePosies_[pLifeUI_->GetMaxLife()- pLifeUI_->GetLife()];
	startPram_.textureWidth = pLifeUI_->GetParamater().kTextureWidth;
	startPram_.textureHeight = pLifeUI_->GetParamater().kTextureHeight;
	startPram_.offSet= pLifeUI_->GetParamater().offsetNormal,

	///endParm
	endParm_.basePos = pLifeUI_->GetParamater().basePosiesDeath[pLifeUI_->GetMaxLife() - pLifeUI_->GetLife()];
	endParm_.textureWidth = pLifeUI_->GetParamater().deathTextureWidth_;
	endParm_.textureHeight = pLifeUI_->GetParamater().deathTextureHeigth_;
	endParm_.offSet = pLifeUI_->GetParamater().offsetDeath,

	kWaitAfterBreakTime_ = 0.4f;

	step_ = Step::WAIT;

	ParticleManager::Load(breakEmit1_, "lifeUIBreak1");
	ParticleManager::Load(breakEmit2_, "lifeUIBreak2");

	breakSE_ = Audio::GetInstance()->SoundLoadWave("UIBreakSE.wav");

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
		pLifeUI_->SetPos       (EaseInCirc(startPram_.basePos,      endParm_.basePos,       deathMoveTime_, pLifeUI_->GetParamater().moveUIEaseTime_));
		pLifeUI_->SetSizeWidth (EaseInCirc(startPram_.textureWidth, endParm_.textureWidth,  deathMoveTime_, pLifeUI_->GetParamater().moveUIEaseTime_));
		pLifeUI_->SetSizeHeigth(EaseInCirc(startPram_.textureHeight,endParm_.textureHeight, deathMoveTime_, pLifeUI_->GetParamater().moveUIEaseTime_));
		pLifeUI_->SetOffSet    (EaseInCirc(startPram_.offSet,       endParm_.offSet,        deathMoveTime_, pLifeUI_->GetParamater().moveUIEaseTime_));

		// 次の
		if(deathMoveTime_ < pLifeUI_->GetParamater().moveUIEaseTime_)break;
		deathMoveTime_ = pLifeUI_->GetParamater().moveUIEaseTime_;
		//parmSet
		pLifeUI_->SetPos       (endParm_.basePos);
		pLifeUI_->SetSizeWidth (endParm_.textureWidth);
		pLifeUI_->SetSizeHeigth(endParm_.textureHeight);
		pLifeUI_->SetOffSet    (endParm_.offSet);

		step_ = Step::WAITBEFOREBREAK;
		break;
		///-------------------------------------------------------
		/// 壊れるまでの待機時間
		///-------------------------------------------------------
	case LifeUIDeath::Step::WAITBEFOREBREAK:
		waitBeforeBreakTime_ += FPSKeeper::NormalDeltaTime();//deltatime加算

		// 次のステップ
		if (waitBeforeBreakTime_ < pLifeUI_->GetParamater().braekWaitTime_)break;
		step_ = Step::BREAK;
		break;
		///-------------------------------------------------------
		/// ライフUI破壊
		///-------------------------------------------------------
	case LifeUIDeath::Step::BREAK:
		pLifeUI_->LifeBreak();// ライフ減らす

		breakEmit1_.pos = pLifeUI_->GetEmitterPos();
		breakEmit2_.pos = pLifeUI_->GetEmitterPos();
		breakEmit1_.Burst();
		breakEmit2_.Burst();
		Audio::GetInstance()->SoundPlayWave(breakSE_,0.25f);
		step_ = Step::WAITAFTERBREAK;
		break;
		///-------------------------------------------------------
		/// 壊れてからの待機
		///-------------------------------------------------------
	case LifeUIDeath::Step::WAITAFTERBREAK:

		waitAfterBreakTime_ += FPSKeeper::NormalDeltaTime();//deltatime加算

		// 次のステップ
		if (waitAfterBreakTime_ < kWaitAfterBreakTime_)break;

		// UI戻す
		if (pLifeUI_->GetLife() > 0) {
			nextSetPos_ = pLifeUI_->GetParamater().basePosies_[pLifeUI_->GetMaxLife() - pLifeUI_->GetLife()];
			step_ = Step::RETURN;
		}
		// 終わる
		else {

			pLifeUI_->SetIsGameOver(true);
			step_ = Step::GOROOT;
		}
		break;
		///-------------------------------------------------------
		/// UI元の位置に戻る
		///-------------------------------------------------------
	case LifeUIDeath::Step::RETURN:
		returnMoveTime_ += FPSKeeper::NormalDeltaTime();

		// イージング適応
		pLifeUI_->SetPos       (EaseInCirc(endParm_.basePos,      nextSetPos_,               returnMoveTime_, pLifeUI_->GetParamater().moveUIEaseTime_));
		pLifeUI_->SetSizeWidth (EaseInCirc(endParm_.textureWidth, startPram_.textureWidth,   returnMoveTime_, pLifeUI_->GetParamater().moveUIEaseTime_));
		pLifeUI_->SetSizeHeigth(EaseInCirc(endParm_.textureHeight, startPram_.textureHeight, returnMoveTime_, pLifeUI_->GetParamater().moveUIEaseTime_));
		pLifeUI_->SetOffSet    (EaseInCirc(endParm_.offSet,        startPram_.offSet,        returnMoveTime_, pLifeUI_->GetParamater().moveUIEaseTime_));


		// 次の
		if (returnMoveTime_ < pLifeUI_->GetParamater().moveUIEaseTime_)break;
		    returnMoveTime_ = pLifeUI_->GetParamater().moveUIEaseTime_;

		//parmSet
		pLifeUI_->SetPos        (nextSetPos_);
		pLifeUI_->SetSizeWidth  (startPram_.textureWidth);
		pLifeUI_->SetSizeHeigth (startPram_.textureHeight);
		pLifeUI_->SetOffSet     (startPram_.offSet);
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

