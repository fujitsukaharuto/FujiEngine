#pragma once
#include "Game/OriginObject/OriginGameObject.h"
#include "Game/Collider/AABBCollider.h"
#include "Engine/Model/Sprite/Sprite.h"
#include "Engine/Audio/AudioPlayer.h"

#include "Game/GameObj/Player/Behavior/BasePlayerBehavior.h"
#include "Game/GameObj/Player/AttackBehavior/BasePlayerAttackBehavior.h"
#include "Game/GameObj/Player/PlayerBullet.h"
#include <vector>


struct DamageStepTime {
	float start;      // この時間以上
	float end;        // この時間未満
	bool popVignette; // ヴィネットを出すステップかどうか
};

struct PlayerHPParams {
	std::vector<DamageStepTime> damageStep;
	float hp;
	float maxHp;
	float damageCoolTime = 60.0f;
};

struct PlayerHPSpritParams {
	Math::Vector4 color = { 0.7f,0.211f,0.505f,1.0f };
	Math::Vector2 hpSize = { 275.0f,35.0f };
	Math::Vector2 hpStartPos = { 55.0f,650.0f };
	Math::Vector2 hpFrameSize = { 285.0f,42.5f };
	Math::Vector2 hpFrameInSize = { 275.0f,35.0f };
	Math::Vector2 hpFrameStartPos = { 192.5f,650.0f };
};

struct PlayerParams {
	PlayerHPParams hp;
	PlayerHPSpritParams hpSprite;
};

/// <summary>
/// Playerクラス
/// </summary>
class Player : public OriginGameObject {
public:
	Player();
	~Player();

	void Initialize()override;
	void Finalize();
	void Update()override;
	void Draw(Graphics::Material* mate = nullptr, bool is = false)override;
	void DebugGUI()override;
	void ParameterGUI();
	void InitParameter();
	void ReStart();

	/// <summary>
	///	HPバーの処理
	/// </summary>
	void HPUpdate();

	//========================================================================*/
	//* Behavior
	void ChangeBehavior(std::unique_ptr<BasePlayerBehavior>behavior);
	void ChangeAttackBehavior(std::unique_ptr<BasePlayerAttackBehavior>behavior);

	//========================================================================*/
	//* Collision
	void OnCollisionEnter([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionStay([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionExit([[maybe_unused]] const ColliderInfo& other);

	//========================================================================*/
	//* Move
	/// <summary>移動処理</summary>
	void Move(const float& speed);
	/// <summary>移動の詳しい処理</summary>
	void MoveTrans(const float& speed);
	/// <summary>移動時の回転</summary>
	void MoveRotate();
	/// <summary>移動する方向</summary>
	Math::Vector3 GetInputDirection();
	/// <summary>移動するかどうか</summary>
	bool GetIsMove();

	//========================================================================*/
	//* Jump
	/// <summary>ジャンプの処理</summary>
	void Jump(float& speed);
	/// <summary>落下</summary>
	void Fall(float& speed);

	//========================================================================*/
	//* Avoid
	/// <summary>回避時の処理</summary>
	void Avoid(float& avoidTime);

	//========================================================================*/
	//* Bullet
	void InitBullet();
	/// <summary>発射時の処理</summary>
	void ReleaseBullet();
	/// <summary>弾の強化</summary>
	void StrengthBullet();

	//========================================================================*/
	//* Landing
	/// <summary>着陸処理</summary>
	void LandingUpdate();

	//========================================================================*/
	//* Getter
	float GetMoveSpeed() { return moveSpeed_; }
	float GetJumpSpeed() { return jumpSpeed_; }
	float GetSecondJumpSpeed() { return secondJumpSpeed_; }
	float GetFallSpeed() { return fallSpeed_; }
	float GetMaxChargeTime() { return maxChargeTime_; }
	float GetAvoidCoolTime() { return avoidCoolTime_; }
	int GetGrabDir() { return dir_; }
	const Math::Vector3& GetLandingStartPos() { return titleStartP_; }
	bool GetIsFall() { return isFall_; }
	bool GetIsGameOver() { return isGameOver_; }
	bool GetIsStart() { return isStart_; }
	bool GetIsStrongState() { return isStrongState_; }
	BaseCollider* GetCollider() { return collider_.get(); }

	std::vector<std::unique_ptr<PlayerBullet>>& GetPlayerBullet() { return bullets_; }

	//========================================================================*/
	//* Setter
	void SetFallSpeed(float speed) { fallSpeed_ = speed; }
	void SetIsFall(bool is) { isFall_ = is; }
	void SetIsStart(bool is) { isStart_ = is; }
	void SetTargetPos(const Math::Vector3& pos) { targetPos_ = pos; }
	void SetIsNowAvoid(bool is) { isNowAvoid_ = is; }
	void SetDXCom(DXCom* dxcom) { dxcommon_ = dxcom; }
	void SetLandingTime(float landTime);

	//========================================================================*/
	//* Title
	void TitleUpdate(float titleTime);
	void TitleDraw();
	void SettingTitleStartPosition(const Math::Vector3& start, const Math::Vector3& center, const Math::Vector3& end);

	void TitleStartUpdate(float titleTime);


private:

	void ParticleEmitterSetting();
	void MoveEngineParticle();
	void InitAvoidPostEffect();
	void AvoidPostEffect();
	void AvoidSetting();
	void DamageSetting();
	void HPUnderZeroSetting();

private:

	std::unique_ptr<BasePlayerBehavior> behavior_ = nullptr;
	std::unique_ptr<BasePlayerAttackBehavior> attackBehavior_ = nullptr;
	std::vector<std::unique_ptr<PlayerBullet>> bullets_;

	std::unique_ptr<Graphics::Object3d> shadow_;
	std::unique_ptr<Graphics::Object3d> strongStatePos_;
	std::unique_ptr<AABBCollider> collider_;

	Math::Vector3 targetPos_;

	PlayerParams params_;

	bool isDamage_ = false;
	float damageCoolTime_ = 30.0f;
	std::unique_ptr<Graphics::Sprite> hpSprite_;
	std::vector<std::unique_ptr<Graphics::Sprite>> hpFrame_;
	Math::Vector3 damageColor_ = { 1.5f,1.5f,5.0f };

	float moveSpeed_ = 0.0f;
	float jumpSpeed_ = 0.0f;
	float secondJumpSpeed_ = 0.0f;
	float fallSpeed_ = 0.0f;
	float gravity_ = 0.0f;
	float maxFallSpeed_ = 0.0f;
	float maxChargeTime_ = 0.0f;
	bool isFall_ = false;

	float avoidRotate_ = 0.0f;
	float avoidDirection_ = 1.0f;
	float avoidCoolTime_ = 0.0f;
	float avoidEffectTime_ = 0.0f;
	float avoidEffectBaseTime_ = 15.0f;
	bool isNowAvoid_ = false;
	bool isCanStrongState_ = false;
	bool isStrongState_ = false;

	bool isStart_ = true;
	bool isDeath_ = false;
	float deathTime_ = 240.0f;
	bool isGameOver_ = false;

	Math::Vector3 velocity_{};
	Math::Vector3 inputDirection_{};
	int dir_ = 0;

	float startLandingTime_ = 0.0f;
	float startLandingMax_ = 0.0f;

	int moveParticleCSL_;
	int moveParticleCSR_;

	DXCom* dxcommon_ = nullptr;

	ParticleEmitter hit_{};
	ParticleEmitter hit2_{};
	ParticleEmitter moveParticleL_{};
	ParticleEmitter moveParticleR_{};
	ParticleEmitter deathSmoke_{};
	ParticleEmitter shotSpark_{};
	ParticleEmitter shotWave_{};
	ParticleEmitter strongShotWave_{};

	ParticleEmitter* moveBurnerL_{};
	ParticleEmitter* moveBurnerR_{};
	ParticleEmitter* moveBurnerLT_{};
	ParticleEmitter* moveBurnerRT_{};

	ParticleEmitter* avoidEmitter01_{};
	ParticleEmitter* avoidEmitter02_{};
	ParticleEmitter* avoidEmitter03_{};

	ParticleEmitter* avoidEmitter1_{};
	ParticleEmitter* avoidEmitter2_{};
	ParticleEmitter* avoidEmitter3_{};
	ParticleEmitter* avoidEmitter4_{};

	ParticleEmitter* strengthStateEmitter1_{};
	ParticleEmitter strengthStateEmitter2_{};

	SoundData* shotSE_{};

	// Title
	Math::Vector3 titleStartP_{};
	Math::Vector3 titleCenterP_{};
	Math::Vector3 titleEndP_{};
	float preTitleTime_ = 300.0f;

};
