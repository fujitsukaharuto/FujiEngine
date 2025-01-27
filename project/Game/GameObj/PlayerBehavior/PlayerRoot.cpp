/// behavior
#include"PlayerRoot.h"
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
PlayerRoot::PlayerRoot(Player* boss)
	: BasePlayerBehavior("PlayerRoot", boss) {

	/// ===================================================
	/// 変数初期化
	/// ===================================================
	

}

PlayerRoot ::~PlayerRoot() {

}

//更新
void PlayerRoot::Update() {

	pPlayer_->Move(pPlayer_->GetParamater().moveSpeed_);

	//　ジャンプに切り替え
	if (Input::GetInstance()->PushKey(DIK_J)) {
		pPlayer_->ChangeBehavior(std::make_unique<PlayerJump>(pPlayer_));
		return;
	}
	else {
		JumpForJoyState();//コントローラジャンプ
		return;
	}

	
}

void PlayerRoot::JumpForJoyState() {
	bool isMoving = false;
	const float thresholdValue = 0.8f;
	Vector3 StickVelocity;

	// 移動量の計算（スティックの入力値を正規化）
	StickVelocity = { 0.0f, (float)joyState.Gamepad.sThumbLY / SHRT_MAX, 0.0f };

	// スティックの移動が一定値を超えているかチェック
	if (StickVelocity.Length() > thresholdValue) {
		isMoving = true;
	}

	// ジャンプの条件を追加
	// Aボタンが押されている、またはスティックが一定以上上方向に倒されている場合にジャンプ
	if ((joyState.Gamepad.wButtons & XINPUT_GAMEPAD_A)       || 
		(joyState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) ||
		isMoving) {
		pPlayer_->ChangeBehavior(std::make_unique<PlayerJump>(pPlayer_));
	}
}
 



void  PlayerRoot::Debug() {
#ifdef _DEBUG
	ImGui::Text("Root");
#endif // _DEBUG	
}

