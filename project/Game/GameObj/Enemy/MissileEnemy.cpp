#include "MissileEnemy.h"
#include"Behavior/EnemyFall.h"
#include"Particle/ParticleManager.h"

///========================================================
///  初期化
///========================================================
void MissileEnemy::Initialize() {
	BaseEnemy::Initialize();
	OriginGameObject::CreateModel("StrongEnemy.obj");

	ParticleManager::Load(bombEmit1_, "MisilleBomb1");
	ParticleManager::Load(bombEmit2_, "MisilleBomb2");
	ParticleManager::Load(bombEmit3_, "bombShock1");
	ParticleManager::Load(bombEmit4_, "bombShock2");

	bombSE_ = Audio::GetInstance()->SoundLoadWave("MissileBomb.wav");
	luncherSE_ = Audio::GetInstance()->SoundLoadWave("MissileLauncher.wav");

	firstFallEase_.maxTime = 0.5f;
	sideMoveEase_.maxTime = 0.5f;
	scalingEase_.maxTime = 1.0f;
	easeDirection_ = 1.0f;
	antipationEase_.maxTime = 0.5f;
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
		firstFallEase_.time += FPSKeeper::DeltaTimeRate();

		// イージング適応
		model_->transform.translate.y = EaseInCubic(startPos_.y, paramater_.fallPos, 
			                                        firstFallEase_.time, firstFallEase_.maxTime);

		
		if (firstFallEase_.time < firstFallEase_.maxTime)break;
		model_->transform.translate.y = paramater_.fallPos;
		firstFallEase_.time = firstFallEase_.maxTime;
		step_ = Step::SIDEMOVE;

		break;
		///-------------------------------------------------------------
		///　横移動
		///-------------------------------------------------------------- 
	case MissileEnemy::Step::SIDEMOVE:
		// タイム加算
		sideMoveEase_.time += FPSKeeper::DeltaTimeRate();

		// イージング適応
		model_->transform.translate.x = EaseInCubic(startPos_.x, targetPosX_,
			sideMoveEase_.time, sideMoveEase_.maxTime);

		/// 次のステップ
		if (sideMoveEase_.time < sideMoveEase_.maxTime)break;
		model_->transform.translate.y = paramater_.fallPos;
		sideMoveEase_.time = sideMoveEase_.maxTime;
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
			scalingEase_.maxTime = 0.4f;
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
		antipationEase_.time += FPSKeeper::DeltaTimeRate();

		// イージング適応
		model_->transform.translate.y = EaseInCubic(paramater_.fallPos, paramater_.fallPos+paramater_.antipationOffsetPos_,
			antipationEase_.time, antipationEase_.maxTime);

		/// 次のステップ
		if (antipationEase_.time < antipationEase_.maxTime)break;
		model_->transform.translate.y = paramater_.fallPos + paramater_.antipationOffsetPos_;
		antipationEase_.time = antipationEase_.maxTime;
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
	/*/// emitter

	bombEmit1_.pos = model_->GetWorldPos();
	bombEmit2_.pos = model_->GetWorldPos();
	bombEmit3_.pos = model_->GetWorldPos();
	bombEmit4_.pos = model_->GetWorldPos();

	bombEmit1_.Burst();
	bombEmit2_.Burst();
	bombEmit3_.Burst();
	bombEmit4_.Burst();

	Audio::GetInstance()->SoundPlayWave(bombSE_, 0.05f);*/
}
	bomb_ = Audio::GetInstance()->SoundLoadWave("mini_bomb2.wav");
	Audio::GetInstance()->SoundPlayWave(bomb_, 0.05f);*/
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