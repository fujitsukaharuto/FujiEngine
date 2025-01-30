
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
	/// 変数初期化
	///---------------------------------------------------

	// parm
	speed_ = pPlayer_->GetParamater().jumpSpeed_;
	pPlayer_->SetFootMotionTime(0.0f);
	
	//se
	jumpSE_ = Audio::GetInstance()->SoundLoadWave("jump.wav");
	Audio::GetInstance()->SoundPlayWave(jumpSE_);

	//particle
	ParticleManager::Load(jumpEmit_, "jump");
	jumpEmit_.pos = pPlayer_->GetModel()->GetWorldPos();
	jumpEmit_.Burst();

	ParticleManager::Load(landingEmit_, "jumpLanding");
	ParticleManager::Load(flyEmit1_, "flyPlayer1");
	ParticleManager::Load(flyEmit2_, "flyPlayer2");


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

		// time更新
		backStartTime_ += FPSKeeper::DeltaTimeRate();
		footSlopeTime_ += FPSKeeper::DeltaTimeRate();
		footSlopeTime_ = min(footSlopeTime_, pPlayer_->GetParamater().footSlopeTime_);
	
		// 回転イージング
		slopeRotate_ = EaseInSine(0.0f, pPlayer_->GetParamater().jumpFootRotateX_,
			footSlopeTime_, pPlayer_->GetParamater().footSlopeTime_);

		// 回転代入
		for (size_t i = 0; i < 2; i++) {
			pPlayer_->GetFootPartsModel(i)->transform.rotate.x = slopeRotate_;
		}

		// 次のステップ
		if (backStartTime_ < pPlayer_->GetParamater().footBackTime_) break;
		renditionStep_ = RenditionStep::SLOPEBACK;

		break;
		///---------------------------------------------------------------------------------------
		/// 戻る
		///---------------------------------------------------------------------------------------
	case PlayerJump::RenditionStep::SLOPEBACK:

		footBackTime_ += FPSKeeper::DeltaTimeRate();
		backRotate_ = EaseOutBack(pPlayer_->GetParamater().jumpFootRotateX_, 0.0f, footBackTime_, pPlayer_->GetParamater().footSlopeBackTime_);
		// 足の向き
		for (size_t i = 0; i < 2; i++) {
			pPlayer_->GetFootPartsModel(i)->transform.rotate.x = backRotate_;
		}

		//向き戻す
		if (footBackTime_ >= pPlayer_->GetParamater().footSlopeBackTime_) {
			footBackTime_ = pPlayer_->GetParamater().footSlopeBackTime_;
			for (size_t i = 0; i < 2; i++) {
				pPlayer_->GetFootPartsModel(i)->transform.rotate.x = 0.0f;
			}
		}
		break;
	default:
		break;
	}
	
	
	/// ===================================================
	/// 普通処理のステップ
	/// ===================================================
	Vector3 flydirection{};
	switch (step_)
	{
		///---------------------------------------------------------------------------------------
		/// ジャンプ
		///---------------------------------------------------------------------------------------
	case PlayerJump::Step::JUMP:
		
		//移動、ジャンプ
		pPlayer_->Move(pPlayer_->GetParamater().airMoveSpeed_);
		pPlayer_->Jump(speed_);

		flydirection = { pPlayer_->GetFacingDirection(), speed_ * 0.75f ,0.0f };
		flydirection = flydirection.Normalize() * 0.4f;
		flyEmit1_.pos = pPlayer_->GetModel()->GetWorldPos();
		flyEmit2_.pos = pPlayer_->GetModel()->GetWorldPos();
		flyEmit1_.RandomSpeed({ -flydirection.x,-flydirection.x }, { -flydirection.y,-flydirection.y }, { 0.0f,0.0f });
		flyEmit2_.RandomSpeed({ -flydirection.x,-flydirection.x }, { -flydirection.y,-flydirection.y }, { 0.0f,0.0f });
		flyEmit1_.Emit();
		flyEmit2_.Emit();


		// ジャンプ終了
		if (pPlayer_->GetModel()->transform.translate.y > Player::InitY_) break;
		landingEmit_.pos = pPlayer_->GetModel()->GetWorldPos();
		landingEmit_.pos.y -= 3.0f;
		landingEmit_.Burst();
		step_ = Step::RETURNROOT;
		 
		break;
		///---------------------------------------------------------------------------------------
		/// 通常に移行
		///---------------------------------------------------------------------------------------
	case PlayerJump::Step::RETURNROOT:
		
		// 足の向きをゼロに
		for (size_t i = 0; i < 2; i++) {
			pPlayer_->GetFootPartsModel(i)->transform.rotate = {0.0f,0.0f,0.0f};
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