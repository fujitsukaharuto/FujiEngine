#pragma once
#include "Engine/GameObject/GameObject.h"
#include "Engine/Collision/AABBCollider.h"
#include "Engine/Graphics/Particle/ParticleEmitter.h"
#include "Engine/Graphics/Sprite/Sprite.h"
#include "Engine/Audio/AudioPlayer.h"

#include "Game/GameObj/Enemy/Behavior/BaseBossBehavior.h"

#include "Game/GameObj/Enemy/BossCore.h"
#include "Game/GameObj/Enemy/BossItem/BossItemManager.h"
#include "Game/GameObj/Enemy/BossItem/UnderRing.h"
#include "Game/GameObj/Enemy/BossItem/WaveWall.h"
#include "Game/GameObj/Enemy/BossItem/Beam.h"
#include "Game/GameObj/Enemy/BossItem/Arrow.h"
#include "Game/Particle/GameEmitters.h"

class Player;
namespace Graphics { class SphereEmitter; }

/// <summary>
/// 残りHPの段階、値が大きいほど体力が多い
/// </summary>
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
class Boss : public GameObject::GameObject {
public:
	Boss();
	~Boss();

	/// <summary>
	/// ジャンプ攻撃の各フェーズの時間と跳ぶ高さのパラメーター
	/// </summary>
	struct JumpParams {
		float jumpTotalTime = 150.0f;
		float upStart = 120.0f;
		float upEnd = 90.0f;
		float downStart = 70.0f;
		float downEnd = 50.0f;
		float height = 4.0f;
		float groundJudgeHeight = 0.25f;
	};

	/// <summary>
	/// ビーム攻撃のチャージ演出と発射後のエフェクトのパラメーター
	/// </summary>
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

		float minReCalcSize = 3.0f;
	};

	/// <summary>
	/// 攻撃時のラジアルブラーの時間と強さのパラメーター
	/// </summary>
	struct BossRadialParams {
		float baseTime = 40.0f;
		float widthStart = 0.0f;
		float widthEnd = 0.005f;
	};

	/// <summary>
	/// 登場演出の展開時間とボスがせり上がる高さのパラメーター
	/// </summary>
	struct SummonParams {
		float summonExpandTime = 50.0f;
		float summonRadiusStart = 30.0f;
		float summonRadiusEnd = 10.0f;
		float energyTime = 120.0f;
		float energySphereEmitStart = 115.0f;
		float bossRiseStartTime = 60.0f;
		float bossDownPos = -30.0f;
	};

	/// <summary>
	/// ボスの調整項目をまとめたもの、Jsonのセーブとロードの単位になる
	/// </summary>
	struct BossParams {
		JumpParams jump;
		BeamParams beam;
		BossRadialParams radial;
		SummonParams summon;
	};


	void Initialize()override;
	void Update()override;
	void Draw(bool is = false)override;
	void CSDispatch();
	void AnimeDraw();
	void DebugGUI()override;
	void ParameterGUI();

	/// <summary>値の初期化</summary>
	void InitParameter();
	void InitEmitter();
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
	
	void CalcModelDir();

	/// <summary>
	/// Dush攻撃について
	/// </summary>
	void DushInit();
	bool DushCharge(float& t, float maxT,bool& isNear,float range);
	bool DushAttack(bool isNear, float& dushRange, float stopRange);

	/// <summary>波攻撃時の処理</summary>
	void WaveWallAttack();
	/// <summary>追尾攻撃時の処理</summary>
	void ArrowAttack();
	/// <summary>雷攻撃時の避雷針の処理</summary>
	void RodFall();

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
	/// <summary>リングの発生処理</summary>
	void UnderRingEmit();

	//========================================================================*/
	//* Behavior
	void ChangeBehavior(std::unique_ptr<BaseBossBehavior>behavior);

	//========================================================================*/
	//* Collision
	void OnCollisionEnter([[maybe_unused]] const Collision::ColliderInfo& other)override;
	void OnCollisionStay([[maybe_unused]] const Collision::ColliderInfo& other)override;
	void OnCollisionExit([[maybe_unused]] const Collision::ColliderInfo& other)override;

	//========================================================================*/
	//* Getter
	Collision::BaseCollider* GetCoreCollider() { return core_->GetCollider(); }
	BossCore* GetBossCore() { return core_.get(); }
	Beam* GetBeam() { return beam_.get(); }
	const  std::vector<std::unique_ptr<WaveWall>>& GetWalls() { return itemManager_->GetWalls(); }
	const std::vector<std::unique_ptr<Arrow>>& GetArrows() { return itemManager_->GetArrows(); }
	const std::vector<std::unique_ptr<UnderRing>>& GetUnderRings() { return itemManager_->GetUnderRings(); }
	const std::vector<std::pair<std::string, float>>& GetPhaseActionList(int phase) { return phaseList_[phase]; }
	float GetAttackCooldown() { return attackCooldown_; }
	int GetPhaseIndex() { return phaseIndex_; }
	float GetBossHP() { return bossHp_; }
	bool GetIsStart() { return isStart_; }
	bool GetIsSummon() { return isSummon_; }
	bool GetIsClear() { return isClear_; }
	bool GetIsNowDush() { return isNowDush_; }
	bool GetIsDamageLight();
	int GetNowHpIndex() { return nowHpIndex_; }
	float GetChainRate();
	Math::Vector3 GetDefaultPos() { return defaultCorePos_; }
	Math::Vector3 GetFrontPos();
	Math::Vector3 GetDamageLightPos();
	float GetCameraRange() { return cameraRange_; }

	/// <summary>ダメージライトの今の明るさ。点いていなければ0</summary>
	float GetDamageLightIntensity() const;
	float GetDamageLightRadius() const { return damageLightRadius_; }

	float GetCameraFollowSpeed() { return cameraFollowSpeed_; }
	const Math::Vector3& GetSummonCameraRotate() { return summonCameraRotate_; }

	//========================================================================*/
	//* Setter
	void SetPlayer(Player* player) { pPlayer_ = player; itemManager_->SetPlayer(player); }
	void SetDXCom(DXC::DXCom* dxcommon) { dxcommon_ = dxcommon; }
	void SetStartWait(float waitT) { startWaiting_ = waitT; }
	void SetCameraRange(float range) { cameraRange_ = range; }
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

	void InitChargeParent();
	void InitItemParent();

	/// <summary>チャージ/トレース用エミッターを引く</summary>
	Graphics::SphereEmitter& TraceEmitter(int i);
	/// <summary>チャージ/トレース用エミッターを寿命と色範囲を指定して発生開始させる</summary>
	void StartTraceEmitters(float lifeTime, const Math::Vector2& colorX, const Math::Vector2& colorY, const Math::Vector2& colorZ);
	/// <summary>チャージ/トレース用エミッターを全て止める</summary>
	void StopTraceEmitters();

	void UpdateEmitterPos(int i);
	void ShrinkScale(int i, float delta);
	void ChangePhase(float threshold, int indexInc);


private:

	//========================================================================*/
	//* 定数
	static constexpr int kChargeCount_ = 8;               // チャージ/トレース用エミッター・親オブジェクトの数（円周を8分割）
	static constexpr int kHpSegmentCount_ = 5;            // HPバーの分割数
	static constexpr int kHpFrameCount_ = 2;              // HPバー枠の数（外枠・内枠）
	static constexpr float kMaxBossHp_ = 100.0f;          // ボスHPの最大値（初期値）
	static constexpr float kInitAttackCooldown_ = 120.0f; // 攻撃クールタイムの初期値
	static constexpr float kWaveAttackPosZ_ = 4.0f;       // 波攻撃エミッターのZオフセット
	static constexpr float kModelDirFollowRate_ = 0.1f;    // プレイヤーの方を向く速さ（0で固まる 1で即座に向く）
	static constexpr float kTraceEmitterLifeTime_ = 0.6f; // チャージ/トレース用エミッターの寿命（ビーム時のみ beam.emitterLifeTime で上書き）

	//* HP・戦闘
	static constexpr float kStrongDamage_ = 4.0f;          // 強攻撃のダメージ量
	static constexpr float kNormalDamage_ = 2.0f;          // 通常攻撃のダメージ量
	static constexpr float kHpThresholdMax_ = 80.0f;       // Maxフェーズからの移行HP
	static constexpr float kHpThresholdHigh_ = 60.0f;      // Highフェーズからの移行HP
	static constexpr float kHpThresholdHalf_ = 40.0f;      // Halfフェーズからの移行HP
	static constexpr float kHpThresholdLow_ = 30.0f;       // Lowフェーズからの移行HP
	static constexpr float kHpColorRangeWide_ = 20.0f;     // HP色補間の範囲（Max/High/Half）
	static constexpr float kHpColorRangeNarrow_ = 10.0f;   // HP色補間の範囲（Low）
	static constexpr float kHpColorRangeEmpty_ = 30.0f;    // HP色補間の範囲（Empty）
	static constexpr float kHpCoolTime_ = 60.0f;           // フェーズ移行時のクールタイム
	static constexpr float kChainRateStep_ = 0.05f;        // 連続行動ごとのチェインレート増加量

	//* タイミング・移動・カメラ（閾値/重複値）
	static constexpr float kStartTime_ = 300.0f;           // 開始演出の時間
	static constexpr float kRoarShakeTimeMin_ = 40.0f;     // 咆哮シェイクを行う時間の下限
	static constexpr float kRoarShakeTimeMax_ = 200.0f;    // 咆哮シェイクを行う時間の上限
	static constexpr float kWalkSpeed_ = 0.05f;            // 歩行速度
	static constexpr float kCameraRange_ = -25.0f;         // カメラ距離の既定値
	static constexpr float kCameraFollowSpeed_ = 0.2f;     // カメラ追従速度の既定値

	BossStateMachine behavior_;
	std::unique_ptr<BossItemManager> itemManager_ = nullptr;

	std::vector<std::string> actionList_;
	std::vector<std::vector<std::pair<std::string, float>>> phaseList_;
	int phaseIndex_ = 0;

	std::unique_ptr<BossCore> core_;
	std::unique_ptr<Beam> beam_;

	/// <summary>所有権は基底の colliders_ が持つ</summary>
	Collision::AABBCollider* collider_ = nullptr;

	Player* pPlayer_;
	/// <remarks>以下は描画しないTransformアンカー。所有権は基底の anchors_ が持つ</remarks>
	std::vector<Math::Trans*> chargeParents_;
	std::vector<Math::Trans*> traceAnchors_;
	std::vector<int> traceEmitterIndexes_;
	Math::Trans* waveParent_ = nullptr;
	std::vector<Math::Trans*> arrowParents_;

	bool isClear_ = false;
	bool isDying_ = false;
	float dyingTime_ = 240.0f;

	bool isHpActive_ = true;
	bool isShakeSprite_ = false;
	bool isActiveSprite_ = false;
	bool isDamageLight_ = false;
	float bossHp_ = 0.0f;
	float hpCoolTime_ = kHpCoolTime_;
	int nowHpIndex_ = kHpSegmentCount_ - 1;
	float shakeTime_ = 0.0f;
	float baseShakeTime_ = 10.0f;
	float shakeSize_ = 4.0f;
	float hpIndent = 1.0f;
	// ダメージを受けた瞬間に damageLightIntensity_ で点灯し、damageLightTime_ 秒かけて0へ落とす
	float damageLightTime_ = 0.3f;
	float damageLightIntensity_ = 5.0f;
	float damageLightRadius_ = 10.0f;
	float damageLightTimer_ = 0.0f;
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
	float startTime_ = kStartTime_;

	float startWaiting_ = 0.0f;
	float summonCircleExpandTime_ = 50.0f;
	float energyTime_ = 120.0f;
	float energyCoolTime_ = 30.0f;
	int summonIndex_ = Game::kInvalidEmitterIndex;
	float bossYPos_ = 0.0f;
	float frontZ_ = 4.5f;
	float coreY_ = 6.5f;
	Math::Vector3 summonCameraRotate_ = { 0.15f, 0.6f, 0.0f };
	float summonCameraRotateStart_ = 0.15f;
	float summonCameraRotateEnd_ = -0.20f;
	Math::Vector3 defaultCorePos_;

	float attackCooldown_ = 0.0f;
	float chargeTime_ = 120.0f;
	float chargeSize_ = 24.0f;

	int arrowSpawnNum_ = 4;

	float jumpTime_ = 0.0f;
	bool isJumpAttack_ = true;
	float chainRate_ = 0.65f;
	int chainCount_ = 0;

	float cameraRange_ = kCameraRange_;
	float cameraFollowSpeed_ = kCameraFollowSpeed_;

	bool isNowDush_ = false;
	int dushTrailIndex_ = Game::kInvalidEmitterIndex;

	// emitter
	Graphics::ParticleEmitter waveAttack1;
	Graphics::ParticleEmitter waveAttack2;
	Graphics::ParticleEmitter waveAttack3;
	Graphics::ParticleEmitter waveAttack4;
	int waveCSEmitIndex_ = Game::kInvalidEmitterIndex;

	Graphics::ParticleEmitter charge9_;
	Graphics::ParticleEmitter charge10_;
	Graphics::ParticleEmitter charge11_;
	Graphics::ParticleEmitter charge12_;
	Graphics::ParticleEmitter charge13_;
	Graphics::ParticleEmitter charge14_;
	Graphics::ParticleEmitter charge15_;

	Graphics::ParticleEmitter jumpWave_;
	int jumpCSEmitIndex_ = Game::kInvalidEmitterIndex;

	Graphics::ParticleEmitter roaringWave_;
	Graphics::ParticleEmitter roaringParticle_;
	Graphics::ParticleEmitter roaringRing_;

	Graphics::ParticleEmitter summonLightning_;;
	Graphics::ParticleEmitter energySphere_;
	Graphics::ParticleEmitter energyParticle_;

	Graphics::ParticleEmitter dushStartParticle_;
	Graphics::ParticleEmitter dushStartCircle_;
	Graphics::ParticleEmitter dushSmoke_;

	int halfAuraCS_ = Game::kInvalidEmitterIndex;
	int halfSmallAuraCS_ = Game::kInvalidEmitterIndex;
	int leftHandAuraCS_ = Game::kInvalidEmitterIndex;
	int rightHandAuraCS_ = Game::kInvalidEmitterIndex;

	Audio::SoundData* jumpAttackSE_;


	// post effect
	float radialTime_ = 0.0f;
	DXC::DXCom* dxcommon_;

};
