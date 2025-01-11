
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
	ImGui::Text("Jump");
}