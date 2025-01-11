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
     playerDirectionX_ = pPlayer_->GetDirectionX();
    step_ = Step::RECOIL;
}

PlayerRecoil::~PlayerRecoil() {}

/// 更新処理
void PlayerRecoil::Update() {
    switch (step_)
    {
    case Step::RECOIL:
      
        // X軸の逆方向に吹っ飛ぶ
        pPlayer_->AddPosition(Vector3(-playerDirectionX_ * (pPlayer_->GetRecoilSpeed() * FPSKeeper::NormalDeltaTime()), 0, 0));
        pPlayer_->Jump(jumpSpeed_);
        if (pPlayer_->GetTrans().translate.y < Player::InitY_)break;
        step_ = Step::RETUNROOT;
        break;
    case Step::RETUNROOT:
        pPlayer_->ChangeBehavior(std::make_unique<PlayerRoot>(pPlayer_));
        break;
    default:
        break;
    }
   
}

/// デバッグ用
void PlayerRecoil::Debug() {
   
}
