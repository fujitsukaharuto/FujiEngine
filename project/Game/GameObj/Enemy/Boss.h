#pragma once
#include "Game/OriginGameObject.h"
#include "Game/Collider/AABBCollider.h"
#include "Engine/Particle/ParticleEmitter.h"
#include "Engine/Model/Sprite.h"

#include "Game/GameObj/Enemy/Behavior/BaseBossBehavior.h"

#include "Game/GameObj/Enemy/BossCore.h"
#include "Game/GameObj/Enemy/BossItem/UnderRing.h"
#include "Game/GameObj/Enemy/BossItem/WaveWall.h"
#include "Game/GameObj/Enemy/BossItem/Beam.h"
#include "Game/GameObj/Enemy/BossItem/Arrow.h"

class Player;

enum class BossHPState {
	Max = 4,
	High = 3,
	Half = 2,
	Low = 1,
	Empty = 0
};

class Boss : public OriginGameObject {
public:
	Boss();
	~Boss();

	void Initialize()override;
	void Update()override;
	void Draw(Material* mate = nullptr, bool is = false)override;
	void CSDispatch();
	void AnimDraw();
	void DebugGUI()override;
	void ParameterGUI();

	void InitParameter();
	void ReStart();

	void ReduceBossHP(bool isStrong);
	void HPColorSet(float under, float index);
	void ShakeHP();

	void Walk();

	void UpdateWaveWall();
	void WaveWallAttack();

	void UpdateArrows();
	void ArrowAttack();

	void UpdateRod();
	void RodFall();
	void RodUnderRing(const Vector3& emitPos);

	void InitBeam();
	bool BeamCharge();
	void BeamChargeComplete();
	bool BeamAttack();

	void InitJumpAttack();
	bool JumpAttack();
	void UpdateUnderRing();
	void UnderRingEmit();

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
	Beam* GetBeam() { return beam_.get(); }
	std::vector<std::unique_ptr<WaveWall>>& GetWalls() { return walls_; }
	std::vector<std::unique_ptr<Arrow>>& GetArrows() { return arrows_; }
	std::vector<std::unique_ptr<UnderRing>>& GetUnderRings() { return undderRings_; }
	const std::vector<std::pair<std::string, float>>& GetPhaseActionList(int phase) { return phaseList_[phase]; }
	float GetAttackCooldown() { return attackCooldown_; }
	int GetPhaseIndex() { return phaseIndex_; }
	float GetBossHP() { return bossHp_; }
	bool GetIsStart() { return isStart_; }
	bool GetIsClear() { return isClear_; }
	int GetNowHpIndex() { return nowHpIndex_; }
	float GetChainRate() { return chainRate_; }
	Vector3 GetDefoultPos() { return defaultCorePos_; }

	void SetPlayer(Player* player) { pPlayer_ = player; }
	void SetDXCom(DXCom* dxcommon) { dxcommon_ = dxcommon; }

	void SavePhase();
	void LoadPhase();

private:

	void SetDefaultBehavior();

	// radial
	void RadialSetting();
	void RadialUpdate();

	bool EnergyUpdate();
	void InitSummon();
	void ExpandSummon();
	void EnergyTimeUpdate();

private:

	std::unique_ptr<BaseBossBehavior> behavior_ = nullptr;

	std::vector<std::string> actionList_;
	std::vector<std::vector<std::pair<std::string, float>>> phaseList_;
	int phaseIndex_ = 0;

	std::unique_ptr<BossCore> core_;
	std::unique_ptr<Beam> beam_;
	std::vector<std::unique_ptr<WaveWall>> walls_;
	std::vector<std::unique_ptr<Arrow>> arrows_;
	std::vector<std::unique_ptr<Arrow>> rods_;
	std::vector<std::unique_ptr<UnderRing>> undderRings_;

	std::unique_ptr<Object3d> shadow_;
	std::unique_ptr<AABBCollider> collider_;

	Player* pPlayer_;
	std::vector<std::unique_ptr<Object3d>> chargeParents_;
	std::unique_ptr<Object3d> waveParent_;
	std::vector<std::unique_ptr<Object3d>> arrowParents_;

	bool isClear_ = false;
	float dyingTime_ = 240.0f;
	bool isDying_ = false;
	float bossHp_ = 0.0f;
	std::vector<std::unique_ptr<Sprite>> hpSprites_;
	std::vector<std::unique_ptr<Sprite>> hpFrame_;
	bool isHpActive_ = true;
	float hpCooltime_ = 60.0f;
	int nowHpIndex_ = 4;
	bool isShakeSprite_ = false;
	float shakeTime_ = 0.0f;
	float baseShakeTime_ = 10.0f;
	float shakeSize_ = 4.0f;
	Vector2 hpSize_ = { 130.0f,35.0f };
	Vector2 hpStartPos_ = { 660.0f,38.0f };
	Vector2 hpFrameSize_ = { 670.6f,44.5f };
	Vector2 hpFrameInSize_ = { 651.4f,34.3f };
	Vector2 hpFrameStartPos_ = { 922.5f,38.0f };
	Vector4 damageColor1_ = { 0.550f,0.20f,0.25f,1.0f };
	Vector4 damageColor2_ = { 0.500f,0.15f,0.20f,1.0f };
	Vector4 damageColor3_ = { 0.450f,0.10f,0.15f,1.0f };
	Vector4 damageColor4_ = { 0.400f,0.05f,0.10f,1.0f };
	float hpIndent = 1.0f;

	bool isStart_ = true;
	float startTime_ = 300.0f;

	float summonCircleExpandTime_ = 50.0f;
	float energyTime_ = 120.0f;
	float energyCoolTime_ = 30.0f;
	int summonIndex_ = 0;
	float summonRadius_ = 0.0f;
	float bossYPos_ = 0.0f;
	Vector3 defaultCorePos_;

	float attackCooldown_ = 0.0f;
	float chargeTime_ = 120.0f;
	float chargeSize_ = 12.0f;
	float baseChargeSize_ = 12.0f;

	float jumpTime_ = 0.0f;
	float jumpHeight_ = 0.0f;
	bool isJumpAttack_ = true;
	float chainRate_ = 0.65f;

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

	ParticleEmitter jumpWave_;

	ParticleEmitter roringWave_;
	ParticleEmitter roringParticle_;
	ParticleEmitter roringring_;

	ParticleEmitter summonLightning_;;
	ParticleEmitter energySphere_;
	ParticleEmitter energyParticle_;



	// post effect
	float radialwidth_ = 0.0f;
	float radialtime_ = 0.0f;
	float baseRadialtime_ = 40.0f;
	DXCom* dxcommon_;

};
