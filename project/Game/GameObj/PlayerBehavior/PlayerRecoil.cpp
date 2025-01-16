#include "PlayerRecoil.h"
#include "PlayerRoot.h"
#include"PlayerJump.h"

#include "Game/GameObj/Player/Player.h"
#include"Game/GameObj/JoyState/JoyState.h"
#include "DX/FPSKeeper.h"

/// ===================================================
/// 初期化
/// ===================================================
PlayerRecoil::PlayerRecoil(Player* player)
	: BasePlayerBehavior("PlayerJump", player) {

	///---------------------------------------------------
	/// 変数初期化
	///---------------------------------------------------
	jumpSpeed_ = pPlayer_->GetRecoilJumpSpeed();
	// プレイヤーの向き (X軸の方向) を取得
	step_ = Step::RECOIL;
}

PlayerRecoil::~PlayerRecoil() {}

/// 更新処理
void PlayerRecoil::Update() {
	switch (step_)
	{
	case Step::RECOIL:
	 ///-------------------------------------------------------------------------------------
	 /// 反動
	 ///-------------------------------------------------------------------------------------

		/// X軸の逆方向に吹っ飛ぶ
		pPlayer_->Jump(jumpSpeed_);
		pPlayer_->Move(pPlayer_->GetAirMoveSpeed());

		/// 地面につく
		if (pPlayer_->GetTrans().translate.y > Player::InitY_)break;
		pPlayer_->GetTrans().translate.y = Player::InitY_;

		step_ = Step::RETUNROOT;

		break;
	case Step::RETUNROOT:
	///-------------------------------------------------------------------------------------
	/// 通常に戻る
	///-------------------------------------------------------------------------------------

		pPlayer_->ChangeBehavior(std::make_unique<PlayerRoot>(pPlayer_));
		return;
		break;

	default:
		break;
	}
	if (this) {
		JumpForBotton();
	}
}

void PlayerRecoil::JumpForBotton() {
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

void PlayerRecoil::JumpForJoyState() {

	if (!(Input::GetInstance()->GetGamepadState(joyState))) return;

	if (!((joyState.Gamepad.wButtons & XINPUT_GAMEPAD_A))) return;

	pPlayer_->ChangeBehavior(std::make_unique<PlayerJump>(pPlayer_));

}


/// デバッグ用
void PlayerRecoil::Debug() {

}
