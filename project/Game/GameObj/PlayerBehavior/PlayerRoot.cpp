/// behavior
#include"PlayerRoot.h"
#include"PlayerJump.h"

/// boss
#include"Game/Player/Player.h"
/// frame
#include"myEngine/Frame/Frame.h"
/// inupt
#include"myEngine/input/Input.h"

//初期化
PlayerRoot::PlayerRoot(Player* boss)
	: BasePlayerBehavior("PlayerRoot", boss) {

	/// ===================================================
	///変数初期化
	/// ===================================================
	

}

PlayerRoot ::~PlayerRoot() {

}

//更新
void PlayerRoot::Update() {

	pPlayer_->Move(0.3f);

	if (Input::GetInstance()->TriggerKey(DIK_J)) {
		pPlayer_->ChangeBehavior(std::make_unique<PlayerJump>(pPlayer_));
	}
}


void  PlayerRoot::Debug() {
	ImGui::Text("Root");
	
}

