
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
UFOPopWait::UFOPopWait(UFO* player)
	: BaseUFOState("UFOPopWait", player) {

	///---------------------------------------------------
	///変数初期化
	///---------------------------------------------------
	easing_.time = 0.0f;
	easing_.maxTime = 0.5f;
	easing_.amplitude = 0.4f;
	easing_.period = 0.2f;
	step_ = Step::POP;
	
}

UFOPopWait ::~UFOPopWait() {

}

//更新
void UFOPopWait::Update() {
	
}


void  UFOPopWait::Debug() {
#ifdef _DEBUG
	ImGui::Text("Jump");
#endif
}