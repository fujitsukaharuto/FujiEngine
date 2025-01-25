#pragma once

///behavior
#include"State/BaseFieldBlockState.h"
#include"Behavior/BaseFieldBlockBehavior.h"

#include"GlobalVariables.h"
// class
#include "OriginGameObject.h"
#include"Collider/AABBCollider.h"
#include<array>

class Player;
class FieldBlock : public OriginGameObject {
	
public:

	enum class Tag {
		NORMAL,
		DAUNGEROUS,
	};

	// パラメータ
	struct Paramater {
		int   breakCount;
		float restoreTime_;
		float damageCollTime_;
	};

protected:

	/// other class
	Player*            pPlayer_;


	/// グローバルなパラメータ
	GlobalVariables* globalParameter_;            /// グローバルパラメータ
	std::string groupName_;

			           
	/// parm		
	std::array<std::string, 2>tags_;
	std::array<std::string, 2>modelNames_;
	Paramater          paramater_;     //パラメータ
	bool               isBreak_;
	uint32_t           serialNum_;
	Vector3            colliderSize_;
	int                hp_;
	
	/// behavior
	std::unique_ptr<BaseFieldBlockState> state_ = nullptr;
	std::unique_ptr<BaseFieldBlockBehavior>behavior_ = nullptr;
	std::unique_ptr<AABBCollider>collider_;
public:
	static uint32_t nextSerialNum_;
public:
	FieldBlock();

	///========================================================================================
	///  public method
	///========================================================================================

	/// 初期化、更新、描画
	virtual void Initialize() override;
	virtual void Update()     override;
	virtual void Draw  (Material* mate = nullptr)override;

	void ChangeState (std::unique_ptr<BaseFieldBlockState>behavior);
	void ChangeBehavior(std::unique_ptr<BaseFieldBlockBehavior>behavior);

	void          OnCollisionEnter (const ColliderInfo& other);
	void          OnCollisionStay  (const ColliderInfo& other);
	BaseCollider* GetCollider      () { return collider_.get(); }

	///-------------------------------------------------------------------------------------
    ///Editor
    ///-------------------------------------------------------------------------------------
	void ParmLoadForImGui();
	void AddParmGroup();
	void SetValues();
	void ApplyGlobalParameter();
	void AdjustParm();


	///========================================================================================
	///  getter method
	///========================================================================================
	Player*            GetPlayer()                         { return pPlayer_; }
	bool               GetIsBreak()const                   { return isBreak_; }
	BaseFieldBlockState* GetBehavior()const                { return state_.get(); }
	Paramater          GetParamater()const                 { return paramater_; }
	Vector3            GetCollisionSize()const             { return colliderSize_; }
	int                GetHp()const                        { return hp_; }
	///========================================================================================
	///  setter method
	///========================================================================================
	void               SetPlayer(Player* plyaer);
	void               SetIsBreak(const bool& is) { isBreak_ = is; };
	void               SetCollisionSize(const Vector3& size);
	void               SetTag(const int& i);
	void               DecrementHp() { hp_--; }
	void               SetHp(const int& hp) { hp_ = hp; }
	void SetParm(const Paramater& paramater);
	void ChangeModel(const int& i);
};
