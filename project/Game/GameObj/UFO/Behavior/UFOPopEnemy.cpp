
///* behavior
#include"UFOPopEnemy.h"
#include"UFORoot.h"

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
UFOPopEnemy::UFOPopEnemy(UFO* player)
	: BaseUFOBehavior("UFOPopEnemy", player) {

	///---------------------------------------------------
	///変数初期化
	///---------------------------------------------------
	easing_.time = 0.0f;
	easing_.maxTime = 0.5f;
	easing_.amplitude = 0.4f;
	easing_.period = 0.2f;
	step_ = Step::POP;
	
}

UFOPopEnemy ::~UFOPopEnemy() {

}

//更新
void UFOPopEnemy::Update() {
	switch (step_)
	{
	case UFOPopEnemy::Step::POP:
		///-------------------------------------------------------
		///　敵生成
		///-------------------------------------------------------
		pUFO_->EnemySpawn();
		step_ = Step::ANIMATION;

		break;

	case UFOPopEnemy::Step::ANIMATION:
		///-------------------------------------------------------
		///　アニメーション演出
		///-------------------------------------------------------
		
		easing_.time += FPSKeeper::NormalDeltaTime();
		pUFO_->SetScale(EaseAmplitudeScale(
			Vector3(10, 1, 1), easing_.time, easing_.maxTime, easing_.amplitude, easing_.period)
		);
		// ステップ遷移
		if (easing_.time < easing_.maxTime)break;
		easing_.time = easing_.maxTime;
		step_ = Step::RETURNROOT;

		break;
	case UFOPopEnemy::Step::RETURNROOT:
		///-------------------------------------------------------
		///　通常に戻るお
		///-------------------------------------------------------
		pUFO_->ChangeBehavior(std::make_unique<UFORoot>(pUFO_));
		break;
	default:
		break;
	}
}


void  UFOPopEnemy::Debug() {
#ifdef _DEBUG
	ImGui::Text("Jump");
#endif
}