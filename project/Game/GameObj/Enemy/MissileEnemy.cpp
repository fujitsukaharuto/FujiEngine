#include "MissileEnemy.h"
#include"Behavior/EnemyFall.h"
#include"Particle/ParticleManager.h"

///========================================================
///  初期化
///========================================================
void MissileEnemy::Initialize() {
	BaseEnemy::Initialize();
	OriginGameObject::CreateModel("Missile.obj");

	ParticleManager::Load(bombEmit1_, "MisilleBomb1");
	ParticleManager::Load(bombEmit2_, "MisilleBomb2");
	ParticleManager::Load(bombEmit3_, "bombShock1");
	ParticleManager::Load(bombEmit4_, "bombShock2");

	bombSE_ = Audio::GetInstance()->SoundLoadWave("MissileBomb.wav");
	luncherSE_ = Audio::GetInstance()->SoundLoadWave("MissileLauncher.wav");

	scalingEase_.maxTime = paramater_.scalingEaseMax;
	model_->transform.scale = paramater_.baseScale;

	// collider
	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetTag(tags_[static_cast<size_t>(Tag::FALL)]);
	SetCollisionSize(BaseScale_ * 2.0f);
	collider_->SetParent(model_.get());
	collider_->InfoUpdate();

	
	step_ = Step::FIRSTFALL;
	ChangeBehavior(std::make_unique<EnemyFall>(this)); /// 追っかけ
}

///========================================================
/// 更新
///========================================================
void MissileEnemy::Update() {
	BaseEnemy::Update();
}

///========================================================
/// 描画
///========================================================
void MissileEnemy::Draw(Material* material) {
	BaseEnemy::Draw(material);
}


void MissileEnemy::FallMove() {

	switch (step_)
	{
		///-------------------------------------------------------------
		///　最初の落下
		///-------------------------------------------------------------- 
	case MissileEnemy::Step::FIRSTFALL:
		// タイム加算
		firstFallEaseT_ += FPSKeeper::DeltaTimeRate();

		// イージング適応
		model_->transform.translate.y = EaseInCubic(startPos_.y, paramater_.fallPos, 
			                                        firstFallEaseT_,paramater_.firstFallEaseMax);

		
		if (firstFallEaseT_ < paramater_.firstFallEaseMax)break;
		model_->transform.translate.y = paramater_.fallPos;
		firstFallEaseT_ = paramater_.firstFallEaseMax;
		step_ = Step::SIDEMOVE;

		break;
		///-------------------------------------------------------------
		///　横移動
		///-------------------------------------------------------------- 
	case MissileEnemy::Step::SIDEMOVE:
		// タイム加算
		sideMoveEaseT_ += FPSKeeper::DeltaTimeRate();

		// イージング適応
		model_->transform.translate.x = EaseInCubic(startPos_.x, targetPosX_,
			sideMoveEaseT_, paramater_.sideMoveEaseMax);

		/// 次のステップ
		if (sideMoveEaseT_ < paramater_.sideMoveEaseMax)break;
		model_->transform.translate.y = paramater_.fallPos;
		sideMoveEaseT_ = paramater_.sideMoveEaseMax;
		step_ = Step::FALLWAIT;
		break;
		///-------------------------------------------------------------
		///　待機
		///-------------------------------------------------------------- 
	case MissileEnemy::Step::FALLWAIT:
		ScalingEaseing();// スケーリング
		// タイム加算
		waitTime_ += FPSKeeper::NormalDeltaTime();

		// タイムにより膨張速度上昇
		if (waitTime_ >= paramater_.scalingUpTime) {
			scalingEase_.maxTime = paramater_.scalingEaseMaxAsLimit;
		}
		/// 次のステップ
		if (waitTime_ < paramater_.fallWaitTime_)break;
		model_->transform.scale = paramater_.baseScale;
		step_ = Step::ANTIPATION;
		break;
		///-------------------------------------------------------------
		///　落下
		///-------------------------------------------------------------- 
	case MissileEnemy::Step::ANTIPATION:
		// タイム加算
		antipationEaseT_ += FPSKeeper::DeltaTimeRate();

		// イージング適応
		model_->transform.translate.y = EaseInCubic(paramater_.fallPos, paramater_.fallPos+paramater_.antipationOffsetPos_,
			antipationEaseT_, paramater_.antipationEaseMax);

		/// 次のステップ
		if (antipationEaseT_ < paramater_.antipationEaseMax)break;
		model_->transform.translate.y = paramater_.fallPos + paramater_.antipationOffsetPos_;
		antipationEaseT_ = paramater_.antipationEaseMax;
		FallSE();//落ちるSE
		step_ = Step::FALL;
		break;
		///-------------------------------------------------------------
		///　落下
		///-------------------------------------------------------------- 
	case MissileEnemy::Step::FALL:
		model_->transform.translate.y -= BaseEnemy::paramater_.fallSpeed* FPSKeeper::DeltaTimeRate();

		break;
	default:
		break;
	}
}


// 爆破演出
void MissileEnemy::ExplotionRendition() {
	/// emitter

	bombEmit1_.pos = model_->GetWorldPos();
	bombEmit2_.pos = model_->GetWorldPos();
	bombEmit3_.pos = model_->GetWorldPos();
	bombEmit4_.pos = model_->GetWorldPos();

	bombEmit1_.Burst();
	bombEmit2_.Burst();
	bombEmit3_.Burst();
	bombEmit4_.Burst();


	Audio::GetInstance()->SoundPlayWave(bombSE_, 0.08f);
}

void MissileEnemy::SetOnlyParamater(const Paramater& parm) {
	paramater_ = parm;
}


void MissileEnemy::ScalingEaseing() {
	// イージングタイムを更新
	scalingEase_.time += FPSKeeper::NormalDeltaTime() * easeDirection_; // 方向に応じて時間を増減

	// タイムが1を超えたら逆方向に、0未満になったら進む方向に変更
	if (scalingEase_.time >= scalingEase_.maxTime) {
		scalingEase_.time = scalingEase_.maxTime;
		easeDirection_ = -1.0f; // 逆方向に切り替え
	}
	else if (scalingEase_.time <= 0.0f) {
		scalingEase_.time = 0.0f;
		easeDirection_ = 1.0f; // 進む方向に切り替え
	}

	model_->transform.scale = EaseInCubic(paramater_.baseScale, paramater_.expansionScale, scalingEase_.time, scalingEase_.maxTime);

}

void MissileEnemy::SetPosition() {
	startPos_ = model_->GetWorldPos();
}

void MissileEnemy::FallSE() {
	Audio::GetInstance()->SoundPlayWave(luncherSE_, 0.08f);

}