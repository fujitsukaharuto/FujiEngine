
///* behavior
#include"PlayerJump.h"
#include"PlayerRoot.h"

///* obj
#include"Game/GameObj/Player/Player.h"

///* frame
#include"DX/FPSKeeper.h"
#include "Particle/ParticleManager.h"


/// ===================================================
/// 初期化
/// ===================================================
PlayerJump::PlayerJump(Player* player)
	: BasePlayerBehavior("PlayerJump", player) {

	///---------------------------------------------------
	///変数初期化
	///---------------------------------------------------

	// parm
	speed_ = pPlayer_->GetParamater().jumpSpeed_;
	pPlayer_->SetMotionTime(0.0f);
	
	//se
	jumpSE_ = Audio::GetInstance()->SoundLoadWave("jump.wav");
	Audio::GetInstance()->SoundPlayWave(jumpSE_);

	//particle
	ParticleManager::Load(jumpEmit_, "jump");
	jumpEmit_.pos = pPlayer_->GetModel()->GetWorldPos();
	jumpEmit_.Burst();

	//ease
	footBackEase_.maxTime = 1.5f;

	step_ = Step::JUMP;
	renditionStep_ = RenditionStep::SLOPE;
}

PlayerJump ::~PlayerJump() {

}

//更新
void PlayerJump::Update() {

	/// ===================================================
	/// 演出ステップ
	/// ===================================================
	switch (renditionStep_)
	{
		///---------------------------------------------------------------------------------------
		/// 傾き
		///---------------------------------------------------------------------------------------
	case PlayerJump::RenditionStep::SLOPE:

		backTime_ += FPSKeeper::DeltaTimeRate();

		// 足の向き
		for (size_t i = 0; i < 2; i++) {
			pPlayer_->GetPartsModel(i)->transform.rotate.x = pPlayer_->GetParamater().jumpFootRotateX_;
		}

		// 次のステップ
		if (backTime_ < pPlayer_->GetParamater().footBackTime_) break;
		renditionStep_ = RenditionStep::SLOPEBACK;

		break;
		///---------------------------------------------------------------------------------------
		/// 戻る
		///---------------------------------------------------------------------------------------
	case PlayerJump::RenditionStep::SLOPEBACK:

		footBackEase_.time += FPSKeeper::DeltaTimeRate();
		backRotate_ = EaseOutBack(pPlayer_->GetParamater().jumpFootRotateX_, 0.0f, footBackEase_.time, footBackEase_.maxTime);
		// 足の向き
		for (size_t i = 0; i < 2; i++) {
			pPlayer_->GetPartsModel(i)->transform.rotate.x = backRotate_;
		}

		//向き戻す
		if (footBackEase_.time >= footBackEase_.maxTime) {
			footBackEase_.time = footBackEase_.maxTime;
			for (size_t i = 0; i < 2; i++) {
				pPlayer_->GetPartsModel(i)->transform.rotate.x = 0.0f;
			}
		}
		break;
	default:
		break;
	}
	
	
	/// ===================================================
	/// 普通処理のステップ
	/// ===================================================
	switch (step_)
	{
		///---------------------------------------------------------------------------------------
		/// ジャンプ
		///---------------------------------------------------------------------------------------
	case PlayerJump::Step::JUMP:
		
		//移動、ジャンプ
		pPlayer_->Move(pPlayer_->GetParamater().airMoveSpeed_);
		pPlayer_->Jump(speed_);


		// ジャンプ終了
		if (pPlayer_->GetModel()->transform.translate.y > Player::InitY_) break;
		step_ = Step::RETURNROOT;
		 
		break;
		///---------------------------------------------------------------------------------------
		/// 通常に移行
		///---------------------------------------------------------------------------------------
	case PlayerJump::Step::RETURNROOT:
		
		// 足の向きをゼロに
		for (size_t i = 0; i < 2; i++) {
			pPlayer_->GetPartsModel(i)->transform.rotate = {0.0f,0.0f,0.0f};
		}

		/// プレイヤー状態遷移
		pPlayer_->ChangeBehavior(std::make_unique<PlayerRoot>(pPlayer_));
		break;
	default:
		break;
	}

	
}


void  PlayerJump::Debug() {

}