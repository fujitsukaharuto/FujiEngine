#include "NormalEnemy.h"
#include"Behavior/EnemySpawnFall.h"
#include"Particle/ParticleManager.h"

///========================================================
///  初期化
///========================================================
void NormalEnemy::Initialize() {
	BaseEnemy::Initialize();

	OriginGameObject::CreateModel("bakudan.obj");

	/// fallParamater
	rotateAmplitude_ = 0.2f;
	rotateFrequency_ = 2.0f;
	translateAmplitude_ = 0.05f;
	translateFrequency_ = 2.0f;
	time_ = 0.0f;

	speedEase_.maxTime = 0.7f;
	maxSpeed_ = paramater_.fallSpeed + 9.0f;

	// collider
	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetTag(tags_[static_cast<size_t>(Tag::FALL)]);
	SetCollisionSize(BaseScale_ * 2.0f);
	collider_->SetParent(model_.get());
	collider_->InfoUpdate();

	ChangeBehavior(std::make_unique<EnemySpawnFall>(this)); /// 追っかけ
}

///========================================================
/// 更新
///========================================================

void NormalEnemy::Update() {
	BaseEnemy::Update();
}

///========================================================
/// 描画
///========================================================

void NormalEnemy::Draw(Material* material) {
	BaseEnemy::Draw(material);
}

///　落ちる時の移動
void NormalEnemy::FallMove() {
	SpeedChangeMethod();
	time_ += FPSKeeper::DeltaTimeRate(); // フレーム間の経過時間を加算

	// Sinewave
	float	zRotation_ = rotateAmplitude_ * std::sin(rotateFrequency_ * time_);
	float 	xTranslation_ = translateAmplitude_ * std::sin(translateFrequency_ * time_);

	///* rotate
	SetRotation(Vector3(0, 0, zRotation_)); // 回転を適用

	///* translate
	AddPosition(Vector3(xTranslation_, -fallspeed_ * FPSKeeper::DeltaTimeRate(), 0)); // Y軸方向に落下


}

/// スピード減速
void NormalEnemy::SpeedChangeMethod() {
	speedEase_.time += FPSKeeper::DeltaTimeRate();
	fallspeed_ = EaseInCirc(maxSpeed_, GetParamater().fallSpeed, speedEase_.time, speedEase_.maxTime);

	if (speedEase_.time >= speedEase_.maxTime) {
		speedEase_.time = speedEase_.maxTime;
		fallspeed_ = GetParamater().fallSpeed;
	}
}

// 爆破演出
void NormalEnemy::ExplotionRendition() {
	/// emitter
	ParticleManager::Load(bombEmit1_, "bomb1");
	ParticleManager::Load(bombEmit2_, "bomb2");
	ParticleManager::Load(bombEmit3_, "bomb3");

	bombEmit1_.pos = model_->GetWorldPos();
	bombEmit2_.pos = model_->GetWorldPos();
	bombEmit3_.pos = model_->GetWorldPos();

	bombEmit1_.Burst();
	bombEmit2_.Burst();
	bombEmit3_.Burst();

	bomb_ = Audio::GetInstance()->SoundLoadWave("mini_bomb2.wav");
	Audio::GetInstance()->SoundPlayWave(bomb_, 0.05f);
}