#include "NormalEnemy.h"
#include"Behavior/EnemySpawnFall.h"
#include"Behavior/EnemyFall.h"
#include"Particle/ParticleManager.h"

float NormalEnemy::StartZPos_ = 5.0f;
float NormalEnemy::StartYPos_ = 70.0f;

///========================================================
///  初期化
///========================================================
void NormalEnemy::Initialize() {
	BaseEnemy::Initialize();

	OriginGameObject::CreateModel("bakudan.obj");

	///* 頭のパーツ
	parachuteModel_ = std::make_unique<Object3d>();
	parachuteModel_->Create("Parachute.obj");
	parachuteModel_->SetParent(model_.get());
	parachuteModel_->transform.scale = Vector3::GetZeroVec();
	parachuteModel_->transform.translate.y = 1.5f;

	/// behavior
	behaviorMap_ = {
		{Type::NORMAL,&NormalEnemy::NormalBehavior},
		{Type::LEFTSIDE,&NormalEnemy::LeftSideBehavior},
		{Type::RIGHTSIDE,&NormalEnemy::RightSideBehavior}
	};

	///parachute
	parachuteExpatioinEase_.maxTime = 0.7f;

	/// fallParamater
	rotateAmplitude_ = 0.2f;
	rotateFrequency_ = 2.0f;
	translateAmplitude_ = 3.0f;
	translateFrequency_ = 2.0f;
	time_ = 0.0f;

	speedEase_.maxTime = 0.7f;
	maxSpeed_ = BaseEnemy::paramater_.fallSpeed + 9.0f;

	SetCollisionSize(BaseScale_ * 2.0f);
	collider_->SetParent(model_.get());
	collider_->InfoUpdate();

	// 開始時の振る舞い
	if (type_ == Type::NORMAL) {
		ChangeBehavior(std::make_unique<EnemySpawnFall>(this)); /// 追っかけ
	}
	else {
		ChangeBehavior(std::make_unique<EnemyFall>(this)); /// 追っかけ
	}
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
	if (dynamic_cast<EnemyFall*>(behavior_.get())) {
		parachuteModel_->Draw(material);
	}
}
///--------------------------------------------------------------------------
/// 落ちる動作
///--------------------------------------------------------------------------
void NormalEnemy::FallMove() {

	// パラシュート展開
	ParachuteExpationMethod();

	time_ += FPSKeeper::DeltaTimeRate(); // フレーム間の経過時間を加算
	// Sinewave
	float	zRotation_ = rotateAmplitude_ * std::sin(rotateFrequency_ * time_);


	// タイプによって動きを変える(横移動)
	auto it = behaviorMap_.find(type_);
	if (it != behaviorMap_.end()) {
		(this->*(it->second))();
	}
		///* rotate
		SetRotation(Vector3(0, 0, zRotation_)); // 回転を適用

		model_->transform.translate.y += -fallspeed_ * FPSKeeper::DeltaTimeRate();
}



///--------------------------------------------------------------------------
/// 通常落下X変化
///--------------------------------------------------------------------------
void NormalEnemy::NormalBehavior() {
	//　減速
	SpeedChangeMethod();

	xTranslation_ = translateAmplitude_ * std::sin(translateFrequency_ * time_);
	model_->transform.translate.x += xTranslation_*FPSKeeper::DeltaTimeRate();
}

///--------------------------------------------------------------------------
/// Left落下X変化
///--------------------------------------------------------------------------
void NormalEnemy::LeftSideBehavior() {
	fallspeed_ = GetParamater().fallSpeed;
	xTranslation_ = translateAmplitude_ * std::sin(translateFrequency_ * time_);
	model_->transform.translate.x += (paramater_.moveValue_ + xTranslation_) * FPSKeeper::DeltaTimeRate();
}
///--------------------------------------------------------------------------
/// Right落下X変化
///--------------------------------------------------------------------------
void NormalEnemy::RightSideBehavior() {
	fallspeed_ = GetParamater().fallSpeed;
	xTranslation_ = translateAmplitude_ * std::sin(translateFrequency_ * time_);
	model_->transform.translate.x -= (paramater_.moveValue_ + xTranslation_) * FPSKeeper::DeltaTimeRate();
}

///--------------------------------------------------------------------------
/// スピード減速
///--------------------------------------------------------------------------
void NormalEnemy::SpeedChangeMethod() {
	speedEase_.time += FPSKeeper::DeltaTimeRate();
	fallspeed_ = EaseInCirc(maxSpeed_, GetParamater().fallSpeed, speedEase_.time, speedEase_.maxTime);

	if (speedEase_.time >= speedEase_.maxTime) {
		speedEase_.time = speedEase_.maxTime;
		fallspeed_ = GetParamater().fallSpeed;
	}
}

///--------------------------------------------------------------------------
/// パラシュート展開
///--------------------------------------------------------------------------
void NormalEnemy::ParachuteExpationMethod() {

	if (parachuteExpatioinEase_.time >= parachuteExpatioinEase_.maxTime)return;
	if (parachuteExpatioinEase_.time == 0.0f) {
		parachuteSE_ = Audio::GetInstance()->SoundLoadWave("openParachute.wav");
		Audio::GetInstance()->SoundPlayWave(parachuteSE_, 0.4f);
	}

	parachuteExpatioinEase_.time += FPSKeeper::DeltaTimeRate();

	parachuteModel_->transform.scale = EaseOutBack(
		Vector3::GetZeroVec(), Vector3(1, 1, 1), parachuteExpatioinEase_.time, parachuteExpatioinEase_.maxTime);

	if (parachuteExpatioinEase_.time >= parachuteExpatioinEase_.maxTime) {
		parachuteExpatioinEase_.time = parachuteExpatioinEase_.maxTime;
		parachuteModel_->transform.scale = Vector3(1, 1, 1);
	}
}

///--------------------------------------------------------------------------
/// 爆破演出
///--------------------------------------------------------------------------
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



///--------------------------------------------------------------------------
/// ポジションセット
///--------------------------------------------------------------------------
void NormalEnemy::SetPosition(const Vector3& pos) {
	model_->transform.translate = pos;

	switch (type_)
	{
	case BaseEnemy::Type::NORMAL:
		model_->transform.translate.y = NormalEnemy::StartYPos_;
		model_->transform.translate.z = NormalEnemy::StartZPos_;
		break;
	case BaseEnemy::Type::LEFTSIDE:
		model_->transform.translate.x = paramater_.leftX;
		break;
	case BaseEnemy::Type::RIGHTSIDE:
		model_->transform.translate.x = paramater_.rightX;
		break;
	default:
		break;
	}
}

void NormalEnemy::SetOnlyParamater(const Paramater& parm) {
	paramater_ = parm;
}
