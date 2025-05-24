#pragma once
#include "Game/OriginGameObject.h"
#include "Game/Collider/AABBCollider.h"
#include "Engine/Particle/ParticleEmitter.h"

#include "Game/GameObj/Enemy/Behavior/BaseBossBehavior.h"

#include "Game/GameObj/Enemy/BossCore.h"
#include "Game/GameObj/Enemy/BossItem/WaveWall.h"

class Player;

class Boss : public OriginGameObject {
public:
	Boss();
	~Boss() = default;

	void Initialize()override;
	void Update()override;
	void Draw(Material* mate = nullptr)override;
	void DebugGUI()override;
	void ParameterGUI();

	void InitParameter();

	void UpdateWaveWall();
	void WaveWallAttack();

	//========================================================================*/
	//* Behavior
	void ChangeBehavior(std::unique_ptr<BaseBossBehavior>behavior);

	//========================================================================*/
	//* Collision
	void OnCollisionEnter([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionStay([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionExit([[maybe_unused]] const ColliderInfo& other);


	BaseCollider* GetCollider() { return collider_.get(); }
	BaseCollider* GetCoreCollider() { return core_->GetCollider(); }
	BossCore* GetBossCore() { return core_.get(); }
	float GetAttackCooldown() { return attackCooldown_; }


	void SetPlayer(Player* player) { pPlayer_ = player; }

private:

private:

	std::unique_ptr<BaseBossBehavior> behavior_ = nullptr;

	std::unique_ptr<BossCore> core_;
	std::vector<std::unique_ptr<WaveWall>> walls_;

	std::unique_ptr<Object3d> shadow_;
	std::unique_ptr<AABBCollider> collider_;

	Player* pPlayer_;

	float attackCooldown_ = 0.0f;


	// emitter
	ParticleEmitter waveAttack1;
	ParticleEmitter waveAttack2;
	ParticleEmitter waveAttack3;
	ParticleEmitter waveAttack4;




};
