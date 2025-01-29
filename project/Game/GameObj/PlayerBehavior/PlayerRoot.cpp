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
	
	// ステップ
	step_ = Step::ROOT;

}

PlayerRoot ::~PlayerRoot() {

}

//更新
void PlayerRoot::Update() {

	switch (step_)
	{
	///---------------------------------------------------
	/// 変数初期化
	///---------------------------------------------------
	case PlayerRoot::Step::ROOT:

		// 移動モーション
		if (pPlayer_->GetIsMoving()) {
			pPlayer_->MoveMotionFoot(pPlayer_->GetParamater().moveFootSpeed_);
			pPlayer_->HeadMotion(pPlayer_->GetParamater().headMotionSpeed);
		}

		//移動
		pPlayer_->Move(pPlayer_->GetParamater().moveSpeed_);

		//　ジャンプに切り替え
		if (Input::GetInstance()->PushKey(DIK_J)) {
			step_ = Step::GOJUMP;
			break;
		}
		else {
			JumpForJoyState();//コントローラジャンプ
			break;
		}
		break;
	///---------------------------------------------------
	/// ジャンプ
	///---------------------------------------------------
	case PlayerRoot::Step::GOJUMP:
		pPlayer_->ChangeBehavior(std::make_unique<PlayerJump>(pPlayer_));
		break;
	default:
		break;
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
		step_ = Step::GOJUMP;
	}
}
 



void  PlayerRoot::Debug() {
#ifdef _DEBUG
	ImGui::Text("Root");
#endif // _DEBUG	
}

