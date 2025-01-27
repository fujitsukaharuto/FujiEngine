/// behavior
#include"PlayerNoneState.h"


/// boss
#include"GameObj/Player/Player.h"
/// frame
#include"DX/FPSKeeper.h"
/// inupt
#include"Input/Input.h"
#include"GameObj/JoyState/JoyState.h"


#include<imgui.h>

//初期化
PlayerNoneState::PlayerNoneState(Player* boss)
	: BasePlayerState("PlayerNoneState", boss) {

	/// ===================================================
	/// 変数初期化
	/// ===================================================


}

PlayerNoneState ::~PlayerNoneState() {

}

//更新
void PlayerNoneState::Update() {



}



void  PlayerNoneState::Debug() {
#ifdef _DEBUG
	ImGui::Text("PlayerNoneState");
#endif // _DEBUG	
}

