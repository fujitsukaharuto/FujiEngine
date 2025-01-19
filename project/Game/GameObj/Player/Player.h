#pragma once
#include"Vector3.h"
#include"Collider/AABBCollider.h"
/// behavior
#include"GameObj/PlayerBehavior/BasePlayerBehavior.h"
#include"GameObj/PlayerBehavior/BasePlayerAttackBehavior.h"
#include"GlobalVariables/GlobalVariables.h"
#include"Game/OriginGameObject.h"

#include <memory>

/// <summary>
/// プレイヤークラス
/// </summary>
class Player :public OriginGameObject {

public:
	enum class KikPower {
		WEAK,
		NORMAL,
		MAXPOWER,
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
	};
private:

	/// ===================================================
	///private variaus
	/// ===================================================

	// キック仮モデル
	std::unique_ptr<Object3d>kikModel_;

	/// グローバルなパラメータ
	const std::string groupName_ = "Player";       /// グループ名
	GlobalVariables* globalParameter_;            /// グローバルパラメータ

	///* behavior
	std::unique_ptr<BasePlayerBehavior>       behavior_ = nullptr;
	std::unique_ptr<BasePlayerAttackBehavior> attackBehavior_ = nullptr;


	///* 速度
	Vector3           velocity_;
	Vector3           prePos_;

	///* 目標角度
	float             objectiveAngle_;
	Vector3           inputDirection_;

	Paramater paramater_;

	/// collider
	std::unique_ptr<AABBCollider> kikCollider_;
	std::unique_ptr<AABBCollider> collider_;
	std::array<std::string, 3>tags_;

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
	bool    GetIsMoving();
	Vector3 GetInputVelocity();
	void    MoveToLimit();

	///* ジャンプ
	void Jump(float& speed);
	void SpecialPostJump(float& speed);
	void Fall(float& speed, const bool& isJump = false);
	void SpecialPostFall(float& speed, const bool& isJump = false);
	void ChangeBehavior(std::unique_ptr<BasePlayerBehavior>behavior);
	void ChangeAttackBehavior(std::unique_ptr<BasePlayerAttackBehavior>behavior);


	/// ダメージ
	void DamageRendition();
	void TakeDamage();


	void          OnCollisionEnter(const ColliderInfo& other);
	void          OnCollisionStay(const ColliderInfo& other);
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
	std::string GetTag(int i) { return tags_[i]; }
	/// ===================================================
	/// setter method
	/// ===================================================
	void SetKikIsCollision(const bool& is);
};
