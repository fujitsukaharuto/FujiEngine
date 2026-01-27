#pragma once
#include "Game/OriginObject/OriginGameObject.h"
#include "Game/Collider/AABBCollider.h"
#include "Engine/Particle/ParticleEmitter.h"
#include "Engine/Model/Sprite.h"
#include "Engine/Audio/AudioPlayer.h"

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


/// <summary>
/// Bossクラス
/// </summary>
class Boss : public OriginGameObject {
public:
	Boss();
	~Boss();

	struct JumpParams {
		float jumpTotalTime = 150.0f;
		float upStart = 120.0f;
		float upEnd = 90.0f;
		float downStart = 70.0f;
		float downEnd = 50.0f;
		float height = 4.0f;
		float groundJudgeHeight = 0.25f;
	};

	struct BeamParams {
		float chargeTime = 120.0f;
		float parentRotateStep;

		float parentY = 20.0f;                // 親オブジェクトの高さ
		float randomRotYMin = -1.56f;
		float randomRotYMax = 1.56f;

		float emitterLifeTime = 0.6f;        // 各トレースの寿命
		float radiusGrowSpeed = 0.075f;       // チャージサークルの拡大速度
		float baseChargeSize = 24.0f;

		float shrinkSpeed = 0.55f;            // 縮小スピード
		float rotateSpeedZ = 0.0225f;         // 回転速度
		float sizeDecrease = 2.0f;            // 縮小後のサイズ減衰量
		float lifeTimeDecrease = 0.04f;        // ライフ減衰量

		float startSizeMulX = 3.0f;
		float startSizeMulY = 6.0f;

		float minRecalcSize = 3.0f;
	};

	struct BossRadialParams {
		float baseTime = 40.0f;
		float widthStart = 0.0f;
		float widthEnd = 0.005f;
	};

	struct SummonParams {
		float summonExpandTime = 50.0f;
		float summonRadiusStart = 30.0f;
		float summonRadiusEnd = 10.0f;
		float energyTime = 120.0f;
		float energySphereEmitStart = 115.0f;
		float bossRiseStartTime = 60.0f;
		float bossDownPos = -30.0f;
	};

	struct BossParams {
		JumpParams jump;
		BeamParams beam;
		BossRadialParams radial;
		SummonParams summon;
	};


	void Initialize()override;
	void Update()override;
	void Draw(Graphics::Material* mate = nullptr, bool is = false)override;
	void CSDispatch();
	void AnimDraw();
	void DebugGUI()override;
	void ParameterGUI();

	/// <summary>値の初期化</summary>
	void InitParameter();
	void ReStart();

	/// <summary>HPの減少処理</summary>
	void ReduceBossHP(bool isStrong);
	/// <summary>HPバーの色処理</summary>
	void HPColorSet(float under, float index);
	/// <summary>HPバーのシェイク処理</summary>
	void ShakeHP();

	/// <summary>行動回数のリセット</summary>
	void ResetChainCount() { chainCount_ = 0; }
	/// <summary>行動連続時のカウンター</summary>
	void ChainCount() { chainCount_++; }

	/// <summary>移動処理</summary>
	void Walk();
	
	void CaluModelDir();

	/// <summary>
	/// Dush攻撃について
	/// </summary>
	void DushInit();
	bool DushCharge(float& t, float maxT,bool& isNear,float reng);
	bool DushAttack(bool isNear, float& dushReng, float stopReng);

	/// <summary>波攻撃の更新</summary>
	void UpdateWaveWall();
	/// <summary>波攻撃時の処理</summary>
	void WaveWallAttack();

	/// <summary>追尾攻撃の更新</summary>
	void UpdateArrows();
	/// <summary>追尾攻撃時の処理</summary>
	void ArrowAttack();

	/// <summary>雷攻撃の更新</summary>
	void UpdateRod();
	/// <summary>雷攻撃時の避雷針の処理</summary>
	void RodFall();
	/// <summary>避雷針のリング攻撃</summary>
	void RodUnderRing(const Math::Vector3& emitPos);

	/// <summary>ビームの初期化</summary>
	void InitBeam();
	/// <summary>ビームのチャージ処理</summary>
	bool BeamCharge();
	/// <summary>ビームのチャージ完了時処理</summary>
	void BeamChargeComplete();
	/// <summary>ビーム攻撃の処理</summary>
	bool BeamAttack();

	/// <summary>ジャンプ攻撃の初期化</summary>
	void InitJumpAttack();
	/// <summary>ジャンプ攻撃の処理</summary>
	bool JumpAttack();
	/// <summary>リングの更新</summary>
	void UpdateUnderRing();
	/// <summary>リングの発生処理</summary>
	void UnderRingEmit();

	//========================================================================*/
	//* Behavior
	void ChangeBehavior(std::unique_ptr<BaseBossBehavior>behavior);

	//========================================================================*/
	//* Collision
	void OnCollisionEnter([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionStay([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionExit([[maybe_unused]] const ColliderInfo& other);

	//========================================================================*/
	//* Getter
	BaseCollider* GetCollider() { return collider_.get(); }
	BaseCollider* GetCoreCollider() { return core_->GetCollider(); }
	BossCore* GetBossCore() { return core_.get(); }
	Beam* GetBeam() { return beam_.get(); }
	const  std::vector<std::unique_ptr<WaveWall>>& GetWalls() { return walls_; }
	const std::vector<std::unique_ptr<Arrow>>& GetArrows() { return arrows_; }
	const std::vector<std::unique_ptr<UnderRing>>& GetUnderRings() { return underRings_; }
	const std::vector<std::pair<std::string, float>>& GetPhaseActionList(int phase) { return phaseList_[phase]; }
	float GetAttackCooldown() { return attackCooldown_; }
	int GetPhaseIndex() { return phaseIndex_; }
	float GetBossHP() { return bossHp_; }
	bool GetIsStart() { return isStart_; }
	bool GetIsSummon() { return isSummon_; }
	bool GetIsClear() { return isClear_; }
	bool GetIsNowDush() { return isNowDush_; }
	int GetNowHpIndex() { return nowHpIndex_; }
	float GetChainRate();
	Math::Vector3 GetDefoultPos() { return defaultCorePos_; }
	float GetCameraRang() { return cameraRang_; }
	float GetCameraFollowSpeed() { return cameraFollowSpeed_; }
	const Math::Vector3& GetSummonCameraRotate() { return summonCameraRotate_; }

	//========================================================================*/
	//* Setter
	void SetPlayer(Player* player) { pPlayer_ = player; }
	void SetDXCom(DXCom* dxcommon) { dxcommon_ = dxcommon; }
	void SetSatrtWait(float waitT) { startWaiting_ = waitT; }
	void SetCameraRang(float rang) { cameraRang_ = rang; }
	void SetCameraFollowSpeed(float speed) { cameraFollowSpeed_ = speed; }

	/// <summary>攻撃フェーズの出力</summary>
	void SavePhase();
	/// <summary>攻撃フェーズの読み込み</summary>
	void LoadPhase();

private:

	/// <summary>デフォルト状態にする</summary>
	void SetDefaultBehavior(bool isInvisibleItem = false);

	//========================================================================*/
	//* radial
	void RadialSetting();
	void RadialUpdate();

	/// <summary>出現時エフェクトの更新</summary>
	bool EnergyUpdate();
	/// <summary>出現時の初期化</summary>
	void InitSummon();
	/// <summary>召喚陣の拡大処理</summary>
	void ExpandSummon();
	/// <summary>出現時間の更新</summary>
	void EnergyTimeUpdate();

	void UpdateEmitterPos(int i);
	void ShrinkScale(int i, float delta);
	void ChangePhase(float threshold, int indexInc);

	Math::Vector3 GetFrontOffset(const Math::Vector3& distance, float angle);

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
	std::vector<std::unique_ptr<UnderRing>> underRings_;

	std::unique_ptr<Graphics::Object3d> shadow_;
	std::unique_ptr<AABBCollider> collider_;

	Player* pPlayer_;
	std::vector<std::unique_ptr<Graphics::Object3d>> chargeParents_;
	std::vector<std::unique_ptr<Graphics::Object3d>> traceAnchors_;
	std::vector<int> traceEmitterIndexes_;
	std::unique_ptr<Graphics::Object3d> waveParent_;
	std::vector<std::unique_ptr<Graphics::Object3d>> arrowParents_;

	bool isClear_ = false;
	bool isDying_ = false;
	float dyingTime_ = 240.0f;

	bool isHpActive_ = true;
	bool isShakeSprite_ = false;
	bool isActiveSprite_ = false;
	float bossHp_ = 0.0f;
	float hpCooltime_ = 60.0f;
	int nowHpIndex_ = 4;
	float shakeTime_ = 0.0f;
	float baseShakeTime_ = 10.0f;
	float shakeSize_ = 4.0f;
	float hpIndent = 1.0f;
	std::vector<std::unique_ptr<Graphics::Sprite>> hpSprites_;
	std::vector<std::unique_ptr<Graphics::Sprite>> hpFrame_;
	Math::Vector2 hpSize_ = { 130.0f,35.0f };
	Math::Vector2 hpStartPos_ = { 660.0f,38.0f };
	Math::Vector2 hpFrameSize_ = { 670.6f,44.5f };
	Math::Vector2 hpFrameInSize_ = { 651.4f,34.3f };
	Math::Vector2 hpFrameStartPos_ = { 922.5f,38.0f };
	Math::Vector4 damageColor1_ = { 0.550f,0.20f,0.25f,1.0f };
	Math::Vector4 damageColor2_ = { 0.500f,0.15f,0.20f,1.0f };
	Math::Vector4 damageColor3_ = { 0.450f,0.10f,0.15f,1.0f };
	Math::Vector4 damageColor4_ = { 0.400f,0.05f,0.10f,1.0f };

	BossParams params_;

	bool isStart_ = true;
	bool isSummon_ = false;
	float startTime_ = 300.0f;

	float startWaiting_ = 0.0f;
	float summonCircleExpandTime_ = 50.0f;
	float energyTime_ = 120.0f;
	float energyCoolTime_ = 30.0f;
	int summonIndex_ = 0;
	float bossYPos_ = 0.0f;
	Math::Vector3 summonCameraRotate_ = { 0.15f, 0.6f, 0.0f };
	float summonCameraRotateStart_ = 0.15f;
	float summonCameraRotateEnd_ = -0.20f;
	Math::Vector3 defaultCorePos_;

	float attackCooldown_ = 0.0f;
	float chargeTime_ = 120.0f;
	float chargeSize_ = 24.0f;

	float jumpTime_ = 0.0f;
	bool isJumpAttack_ = true;
	float chainRate_ = 0.65f;
	int chainCount_ = 0;

	float cameraRang_ = -25.0f;
	float cameraFollowSpeed_ = 0.2f;

	bool isNowDush_ = false;
	int dushChargeIndex_ = 0;
	int dushTrailIndex_ = 0;

	// emitter
	ParticleEmitter waveAttack1;
	ParticleEmitter waveAttack2;
	ParticleEmitter waveAttack3;
	ParticleEmitter waveAttack4;
	int waveCSEmitIndex_ = 0;

	ParticleEmitter charge9_;
	ParticleEmitter charge10_;
	ParticleEmitter charge11_;
	ParticleEmitter charge12_;
	ParticleEmitter charge13_;
	ParticleEmitter charge14_;
	ParticleEmitter charge15_;

	ParticleEmitter jumpWave_;
	int jumpCSEmitIndex_ = 0;

	ParticleEmitter roringWave_;
	ParticleEmitter roringParticle_;
	ParticleEmitter roringring_;

	ParticleEmitter summonLightning_;;
	ParticleEmitter energySphere_;
	ParticleEmitter energyParticle_;

	ParticleEmitter dushStartParticle_;
	ParticleEmitter dushStartCircle_;
	ParticleEmitter dushSmoke_;

	int halfAuraCS_ = 0;
	int halfSmallAuraCS_ = 0;
	int leftHandAuraCS_ = 0;
	int rightHandAuraCS_ = 0;

	SoundData* jumpAttackSE_;


	// post effect
	float radialtime_ = 0.0f;
	DXCom* dxcommon_;

};
