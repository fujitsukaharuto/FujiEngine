
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


}

UFORoot ::~UFORoot() {

}

//更新
void UFORoot::Update() {

}


void  UFORoot::Debug() {
	ImGui::Text("Jump");
}