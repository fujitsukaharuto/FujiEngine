#include "MissileEnemy.h"
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