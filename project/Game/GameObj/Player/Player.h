#pragma once
#include"Vector3.h"
#include"Collider/AABBCollider.h"
#include"GameObj/KikDirectionView/KikDirectionView.h"
#include "Particle/ParticleEmitter.h"
/// behavior
#include"GameObj/PlayerBehavior/BasePlayerBehavior.h"
#include"GameObj/PlayerBehavior/BasePlayerAttackBehavior.h"
#include"GameObj/Player/State/BasePlayerState.h"
#include"GlobalVariables/GlobalVariables.h"
#include"Game/OriginGameObject.h"

#include <memory>

/// <summary>
/// プレイヤークラス
/// </summary>

class FieldBlockManager;
class Player :public OriginGameObject {

public:
	enum class KikPower {
		WEAK,
		MAXPOWER,
	};

	enum class Parts {
		LEFT,
		RIGHT,
	};

	struct Paramater {
		float             moveSpeed_;
		float             airMoveSpeed_;
		float             jumpSpeed_;
		float             gravity_;
		float             recoilSpeed_;
		float             recoilJumpSpeed_;
		float             kikTime_;
		float             kikWaitTime_;
		float             maxFallSpeed_;
		float             specialAttackAntiTime_;
		float             specialAttackFallSpeed_;
		float             specialAttackPostJump_;
		float             specialAttackPostGravity_;
		float             specialAttackPostMaxFallSpeed_;
		float             fallSpeed_;
		float             kikDirectionSpeed_;
		float             kikChargeTime_;
		int               deathCount_;
		float             respownWaitTime_;
		float             respownInvincibleTime_;
		Vector3           respownPos_;
		float             kikRotateTime_;
		Vector3           footStartPosLeft_;
		Vector3           footStartPosRight_;
		float             footMotionAmount_;
		float             moveFootSpeed_;
		float             jumpFootSpeed_;
	};
private:

	/// ===================================================
	///private variaus
	/// ===================================================

	FieldBlockManager* pFieldBlockManager_;

	// キック仮モデル
	std::unique_ptr<Object3d>kikModel_;
	std::array<std::unique_ptr<Object3d>, 2>partsModel_;
	std::unique_ptr<KikDirectionView>kikDirectionView_;

	/// グローバルなパラメータ
	const std::string groupName_ = "Player";       /// グループ名
	GlobalVariables* globalParameter_;            /// グローバルパラメータ

	///* behavior,State
	std::unique_ptr<BasePlayerBehavior>       behavior_ = nullptr;
	std::unique_ptr<BasePlayerAttackBehavior> attackBehavior_ = nullptr;
	std::unique_ptr<BasePlayerState>          state_ = nullptr;

	///* 速度
	Vector3           velocity_;
	Vector3           prePos_;

	///* Parm
	float             objectiveAngle_;
	Vector3           inputDirection_;
	Vector3           kikDirection_;
	int               deathCount_;
	Paramater         paramater_;
	float             motionTime_;

	// ダメージ演出
	float elapsedTime_;
	bool  isTransparent_;

	/// collider
	std::unique_ptr<AABBCollider> kikCollider_;
	std::unique_ptr<AABBCollider> collider_;
	std::array<std::string, 2>    tags_;
	Vector3                       collisionSize_;


public:
	static float InitY_;
	Player();
	/// ===================================================
	///public method
	/// ===================================================

	//* 初期化、更新、描画
	void Initialize()                     override;
	void Update()                         override;
	void Draw(Material* mate = nullptr)   override;

	///* 移動
	void    Move(const float& speed);
	void    MoveMotion(const float& moveSpeed);
	bool    GetIsMoving();
	Vector3 GetInputVelocity();
	void    MoveToLimit();

	void ChangeKikDirection();

	///* ジャンプ
	void Jump(float& speed);
	void SpecialPostJump(float& speed);
	void Fall(float& speed, const bool& isJump = false);
	void SpecialPostFall(float& speed, const bool& isJump = false);
	void ChangeBehavior(std::unique_ptr<BasePlayerBehavior>behavior);
	void ChangeAttackBehavior(std::unique_ptr<BasePlayerAttackBehavior>behavior);
	void ChangeState(std::unique_ptr<BasePlayerState>behavior);

	/// ダメージ
	void DamageRendition(const float&interval);
	void TakeDamage();


	void          OnCollisionPlayerEnter(const ColliderInfo& other);
	void          OnCollisionPlayerStay(const ColliderInfo& other);
	void          OnCollisionKikStay(const ColliderInfo& other);
	BaseCollider* GetKikCollider() { return kikCollider_.get(); }
	BaseCollider* GetCollider() { return collider_.get(); }
	///-------------------------------------------------------------------------------------
	/// Editor
	///-------------------------------------------------------------------------------------

	void AdjustParm();
	void ParmLoadForImGui();
	void AddParmGroup();
	void SetValues();
	void ApplyGlobalParameter();


	/// ===================================================
	/// getter method
	/// ===================================================
	Paramater GetParamater()const { return paramater_; }
	float GetFacingDirection()const;
	Vector3 GetKikDirection()const { return kikDirection_; };
	std::string GetTag(int i) { return tags_[i]; }
	/// ===================================================
	/// setter method
	/// ===================================================
	void SetdeathCount(const int& i) { deathCount_ = i; };
	void SetKikIsCollision(const bool& is);
	void SetTag(const int& i);
	void SetDamageRenditionReset();
	void SetMotionTime(const float& t) { motionTime_ = t; }
};
