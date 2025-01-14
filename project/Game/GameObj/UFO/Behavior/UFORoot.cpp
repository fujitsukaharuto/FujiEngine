
///* behavior
#include"UFOPopEnemy.h"
#include"UFORoot.h"

///* obj
#include"Game/GameObj/UFO/UFO.h"

///* frame
#include"DX/FPSKeeper.h"

///* imgui
#ifdef _DEBUG
#include"imgui.h"
#endif 


/// ===================================================
///初期化
/// ===================================================
UFORoot::UFORoot(UFO* player)
	: BaseUFOBehavior("UFORoot", player) {

	///---------------------------------------------------
	///変数初期化
	///---------------------------------------------------
	step_ = Step::WAIT;

}

UFORoot ::~UFORoot() {

}

//更新
void UFORoot::Update() {
	switch (step_)
	{
	case Step::WAIT:
		///-------------------------------------------------------
		///　待機
		///-------------------------------------------------------
		pUFO_->Move();
		waitTime_ += FPSKeeper::NormalDeltaTime();
		if (waitTime_ < pUFO_->GetPopWaitTime())break;
		step_ = Step::CHANGEPOP;
		break;

	case Step::CHANGEPOP:
		///-------------------------------------------------------
		///　popするお
		///-------------------------------------------------------
		pUFO_->ChangeBehavior(std::make_unique<UFOPopEnemy>(pUFO_));
		break;
	default:
		break;
	}

	
}


void  UFORoot::Debug() {
	ImGui::Text("Jump");
}