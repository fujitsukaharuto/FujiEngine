/// behavior
#include"PlayerKikAttack.h"
#include"PlayerJump.h"

/// boss
#include"GameObj/Player/Player.h"
/// frame
#include"DX/FPSKeeper.h"
/// inupt
#include"Input/Input.h"
#include"GameObj/JoyState/JoyState.h"

#include<imgui.h>

//初期化
PlayerKikAttack::PlayerKikAttack(Player* boss)
	: BasePlayerBehavior("PlayerKikAttack", boss) {

	/// ===================================================
	/// 変数初期化
	/// ===================================================
	
	normalKikCollider_ = std::make_unique<AABBCollider>();
	normalKikCollider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	normalKikCollider_->SetTag("NormalKik");
}

PlayerKikAttack ::~PlayerKikAttack() {

}

//更新
void PlayerKikAttack::Update() {

	pPlayer_->Move(pPlayer_->GetMoveSpeed());

	
}



void  PlayerKikAttack::Debug() {
#ifdef _DEBUG
	ImGui::Text("Root");
#endif // _DEBUG	
}


void PlayerKikAttack::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {



}

void PlayerKikAttack::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {

}
