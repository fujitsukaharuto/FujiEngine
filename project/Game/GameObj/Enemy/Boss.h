#pragma once
#include "Game/OriginGameObject.h"
#include "Game/Collider/AABBCollider.h"
#include "Engine/Particle/ParticleEmitter.h"

#include "Game/GameObj/Enemy/Behavior/BaseBossBehavior.h"

#include "Game/GameObj/Enemy/BossCore.h"
#include "Game/GameObj/Enemy/BossItem/WaveWall.h"
#include "Game/GameObj/Enemy/BossItem/Beam.h"

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

	void ReduceBossHP(bool isStrong);

	void Walk();

	void UpdateWaveWall();
	void WaveWallAttack();

	void InitBeam();
	bool BeamCharge();
	void BeamChargeComplete();
	bool BeamAttack();

	void InitJumpAttack();
	bool JumpAttack();

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
	float GetBossHP() { return bossHp_; }

	void SetPlayer(Player* player) { pPlayer_ = player; }

private:

private:

	std::unique_ptr<BaseBossBehavior> behavior_ = nullptr;

	std::unique_ptr<BossCore> core_;
	std::unique_ptr<Beam> beam_;
	std::vector<std::unique_ptr<WaveWall>> walls_;

	std::unique_ptr<Object3d> shadow_;
	std::unique_ptr<AABBCollider> collider_;

	Player* pPlayer_;
	std::vector<std::unique_ptr<Object3d>> chargeParents_;
	std::unique_ptr<Object3d> waveParent_;

	float bossHp_ = 0.0f;

	float attackCooldown_ = 0.0f;
	float chargeTime_ = 120.0f;
	float chargeSize_ = 12.0f;

	float jumpTime_ = 0.0f;
	float jumpHeight_ = 0.0f;

	// emitter
	ParticleEmitter waveAttack1;
	ParticleEmitter waveAttack2;
	ParticleEmitter waveAttack3;
	ParticleEmitter waveAttack4;

	ParticleEmitter charges_[8];
	
	ParticleEmitter charge9_;
	ParticleEmitter charge10_;
	ParticleEmitter charge11_;
	ParticleEmitter charge12_;
	ParticleEmitter charge13_;
	ParticleEmitter charge14_;
	ParticleEmitter charge15_;


};
