#include "MissileEnemy.h"
#include"Particle/ParticleManager.h"

///========================================================
///  初期化
///========================================================
void MissileEnemy::Initialize() {
	BaseEnemy::Initialize();

	OriginGameObject::CreateModel("StrongEnemy.obj");


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
	Audio::GetInstance()->SoundPlayWave(bomb_, 0.05f);*/
}