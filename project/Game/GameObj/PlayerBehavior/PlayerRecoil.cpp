#include "PlayerRecoil.h"
#include "PlayerRoot.h"

#include "Game/GameObj/Player/Player.h"
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

		pPlayer_->SetRotationZ((pPlayer_->GetFacingDirection() * 10.0f)*FPSKeeper::NormalDeltaTime() );

		/// X軸の逆方向に吹っ飛ぶ
		pPlayer_->AddPosition(Vector3(-pPlayer_->GetFacingDirection() * (pPlayer_->GetRecoilSpeed() * FPSKeeper::NormalDeltaTime()), 0, 0));
		pPlayer_->Jump(jumpSpeed_);

		/// 地面につく
		if (pPlayer_->GetTrans().translate.y > Player::InitY_)break;
		pPlayer_->GetTrans().translate.y = Player::InitY_;
		pPlayer_->SetRotationZ(0.0f);

		step_ = Step::RETUNROOT;

		break;
	case Step::RETUNROOT:
	///-------------------------------------------------------------------------------------
	/// 通常に戻る
	///-------------------------------------------------------------------------------------

		pPlayer_->ChangeBehavior(std::make_unique<PlayerRoot>(pPlayer_));
		break;
	default:
		break;
	}

}

/// デバッグ用
void PlayerRecoil::Debug() {

}
