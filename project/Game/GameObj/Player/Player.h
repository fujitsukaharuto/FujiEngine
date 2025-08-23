#pragma once
#include "Game/OriginGameObject.h"
#include "Game/Collider/AABBCollider.h"
#include "Engine/Model/Sprite.h"

#include "Game/GameObj/Player/Behavior/BasePlayerBehavior.h"
#include "Game/GameObj/Player/AttackBehavior/BasePlayerAttackBehavior.h"
#include "Game/GameObj/Player/PlayerBullet.h"
#include <vector>

class Player : public OriginGameObject{
public:
	Player();
	~Player();

	void Initialize()override;
	void Update()override;
	void Draw(Material* mate = nullptr, bool is = false)override;
	void DebugGUI()override;
	void ParameterGUI();
	void ReStart();

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
	void Move(const float& speed);
	void MoveTrans(const float& speed);
	void MoveRotate();
	Vector3 GetInputDirection();
	bool GetIsMove();

	//========================================================================*/
	//* Jump
	void Jump(float& speed);
	void Fall(float& speed);

	//========================================================================*/
	//* Avoid
	void Avoid(float& avoidTime);

	//========================================================================*/
	//* Bullet
	void InitBullet();
	void ReleaseBullet();
	void StrngthBullet();

	//========================================================================*/
	//* Getter
	float GetMoveSpeed() { return moveSpeed_; }
	float GetJumpSpeed() { return jumpSpeed_; }
	float GetSecoundJumpSpeed() { return secoundJumpSpeed_; }
	float GetFallSpeed() { return fallSpeed_; }
	float GetMaxChargeTime() { return maxChargeTime_; }
	float GetAvoidCoolTime() { return avoidCoolTime_; }
	int GetGrabDir() { return dir_; }
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
	void SetTargetPos(const Vector3& pos) { targetPos_ = pos; }
	void SetIsNowAvoid(bool is) { isNowAvoid_ = is; }
	void SetDXCom(DXCom* dxcom) { dxcommon_ = dxcom; }


	//========================================================================*/
	//* Title
	void TitleUpdate(float titleTime);
	void TitleDraw();
	void SettingTitleStartPosition(const Vector3& start, const Vector3& center, const Vector3& end);

	void TitleStartUpdate(float titleTime);


private:

	void EmitterSetting();

private:

	std::unique_ptr<BasePlayerBehavior> behavior_ = nullptr;
	std::unique_ptr<BasePlayerAttackBehavior> attackBehavior_ = nullptr;
	std::vector<std::unique_ptr<PlayerBullet>> bullets_;

	std::unique_ptr<Object3d> shadow_;
	std::unique_ptr<Object3d> strongStatePos_;
	std::unique_ptr<AABBCollider> collider_;

	Vector3 targetPos_;

	float playerHP_;
	Vector3 damageColor_ = { 1.5f,1.5f,5.0f };
	bool isDamage_ = false;
	float damageCoolTime_ = 30.0f;
	std::unique_ptr<Sprite> hpSprite_;
	std::vector<std::unique_ptr<Sprite>> hpFrame_;
	Vector2 hpSize_ = { 275.0f,35.0f };
	Vector2 hpStartPos_ = { 55.0f,650.0f };
	Vector2 hpFrameSize_ = { 285.0f,42.5f };
	Vector2 hpFrameInSize_ = { 275.0f,35.0f };
	Vector2 hpFrameStartPos_ = { 192.5f,650.0f };

	float moveSpeed_;
	float jumpSpeed_;
	float secoundJumpSpeed_;
	float fallSpeed_;
	float gravity_;
	float maxFallSpeed_;
	float maxChargeTime_;
	bool isFall_;

	float avoidRotate_;
	float avoidDirection_ = 1.0f;
	float avoidCoolTime_;
	bool isNowAvoid_ = false;
	bool isCanStrongState_ = false;
	bool isStrongState_ = false;

	bool isStart_ = true;
	bool isDeath_ = false;
	float deathTime_ = 240.0f;
	bool isGameOver_ = false;

	Vector3 velocity_;
	Vector3 inputDirection_;
	int dir_;

	DXCom* dxcommon_;

	ParticleEmitter hit_;
	ParticleEmitter hit2_;
	ParticleEmitter moveParticleL_;
	ParticleEmitter moveParticleR_;
	ParticleEmitter deathSmoke_;
	ParticleEmitter strongShotWave_;
	
	ParticleEmitter* moveBurnerL_;
	ParticleEmitter* moveBurnerR_;
	ParticleEmitter* moveBurnerLT_;
	ParticleEmitter* moveBurnerRT_;

	ParticleEmitter* avoidEmitter01_;
	ParticleEmitter* avoidEmitter02_;
	ParticleEmitter* avoidEmitter03_;

	ParticleEmitter* avoidEmitter1_;
	ParticleEmitter* avoidEmitter2_;
	ParticleEmitter* avoidEmitter3_;

	ParticleEmitter* storongStateEmitter1_;
	ParticleEmitter storongStateEmitter2_;


	// Title
	Vector3 titleStartP_;
	Vector3 titleCenterP_;
	Vector3 titleEndP_;
	float preTitleTime_ = 300.0f;

};
