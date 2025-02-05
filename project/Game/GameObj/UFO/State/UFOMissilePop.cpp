///* behavior
#include"UFOMissilePop.h"
#include"UFOPopWait.h"

///* obj
#include"GameObj/UFO/UFO.h"

///* frame
#include"DX/FPSKeeper.h"

///* imgui
#ifdef _DEBUG
#include"imgui.h"
#endif 

/// ===================================================
///初期化
/// ===================================================
UFOMissilePop::UFOMissilePop(UFO* player)
	: BaseUFOState("UFOMissilePop", player) {

	///---------------------------------------------------
	///変数初期化
	///---------------------------------------------------
	lightUpEasing_.time = 0.0f;
	lightUpEasing_.maxTime = 0.5f;

	lightCloseEasing_.time = 0.0f;
	lightCloseEasing_.maxTime = 0.5f;
	
	initScale_ = pUFO_->GetParamater().initLightScale;
	scaleUnderPop_ = pUFO_->GetParamater().lightScaleUnderPop;

	waitTime_ = 0.0f;
	kWaitTime_ = 0.3f;

	scalingEase_.maxTime = 0.5f;
	scalingEase_.amplitude = 1.0f;
	scalingEase_.period = 0.2f;


	step_ = Step::LIGHTUP;
	
	firstSE_ = Audio::GetInstance()->SoundLoadWave("missileApe.wav");


}

UFOMissilePop ::~UFOMissilePop() {

}

//更新
void UFOMissilePop::Update() {
	switch (step_)
	{
		///-------------------------------------------------------
		///　ライト出す
		///-------------------------------------------------------
	case UFOMissilePop::Step::LIGHTUP:

		/// タイム加算
		lightUpEasing_.time += FPSKeeper::DeltaTimeRate();
		scalingEase_.time += FPSKeeper::DeltaTimeRate();
	
		// イージング適応
		pUFO_->GetUFOLight()->transform.scale =
			EaseInCubic(initScale_, scaleUnderPop_, lightUpEasing_.time,lightUpEasing_.maxTime);
		pUFO_->SetScale(EaseAmplitudeScale(Vector3(1, 1, 1), scalingEase_.time, scalingEase_.maxTime, scalingEase_.amplitude, scalingEase_.period));

		/// 次のステップ
		if (lightUpEasing_.time < lightUpEasing_.maxTime)break;
		lightUpEasing_.time = lightUpEasing_.maxTime;
		Audio::GetInstance()->SoundPlayWave(firstSE_, 0.08f);
		pUFO_->SetScale(Vector3(1, 1, 1));

		step_ = Step::LIGHTCLOSE;
		break;

		///-------------------------------------------------------
		///　待機
		///-------------------------------------------------------
	case UFOMissilePop::Step::WAIT:
		waitTime_ += FPSKeeper::NormalDeltaTime();
		if (waitTime_ < kWaitTime_)break;
		step_ = Step::LIGHTCLOSE;
		break;

		///-------------------------------------------------------
		///　ライト閉じる
		///-------------------------------------------------------
	case UFOMissilePop::Step::LIGHTCLOSE:
		
		/// タイム加算
		lightCloseEasing_.time += FPSKeeper::DeltaTimeRate();

		// イージング適応
		pUFO_->GetUFOLight()->transform.scale =
			EaseInCubic(scaleUnderPop_, initScale_,  lightCloseEasing_.time, lightCloseEasing_.maxTime);

		/// 次のステップ
		if (lightCloseEasing_.time < lightCloseEasing_.maxTime)break;
		lightCloseEasing_.time = lightCloseEasing_.maxTime;
		step_ = Step::RETURNROOT;
		

		break;
		///-------------------------------------------------------
		///　通常に戻るお
		///-------------------------------------------------------
	case UFOMissilePop::Step::RETURNROOT:
		
		pUFO_->ChangeState(std::make_unique<UFOPopWait>(pUFO_));
		break;
	default:
		break;
	}
}


void  UFOMissilePop::Debug() {
#ifdef _DEBUG
	ImGui::Text("Jump");
#endif
}

