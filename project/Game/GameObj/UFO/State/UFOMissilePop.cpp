
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
	easing_.time = 0.0f;
	easing_.maxTime = 0.5f;
	easing_.amplitude = 0.4f;
	easing_.period = 0.2f;

	step_ = Step::POP;
	
}

UFOMissilePop ::~UFOMissilePop() {

}

//更新
void UFOMissilePop::Update() {
	switch (step_)
	{
	case UFOMissilePop::Step::POP:
		///-------------------------------------------------------
		///　敵生成
		///-------------------------------------------------------
		pUFO_->EnemySpawn();
		step_ = Step::ANIMATION;

		break;

	case UFOMissilePop::Step::ANIMATION:
		///-------------------------------------------------------
		///　アニメーション演出
		///-------------------------------------------------------
		
		easing_.time += FPSKeeper::DeltaTimeRate();
		pUFO_->SetScale(EaseAmplitudeScale(
			Vector3(10, 1, 1), easing_.time, easing_.maxTime, easing_.amplitude, easing_.period)
		);
		// ステップ遷移
		if (easing_.time < easing_.maxTime)break;
		easing_.time = easing_.maxTime;
		step_ = Step::RETURNROOT;

		break;
	case UFOMissilePop::Step::RETURNROOT:
		///-------------------------------------------------------
		///　通常に戻るお
		///-------------------------------------------------------
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

