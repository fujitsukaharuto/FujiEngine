
///* behavior
#include"UFOPopEnemy.h"

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
UFOPopEnemy::UFOPopEnemy(UFO* player)
	: BaseUFOBehavior("UFOPopEnemy", player) {

	///---------------------------------------------------
	///変数初期化
	///---------------------------------------------------

	
}

UFOPopEnemy ::~UFOPopEnemy() {

}

//更新
void UFOPopEnemy::Update() {
	
}


void  UFOPopEnemy::Debug() {
	ImGui::Text("Jump");
}