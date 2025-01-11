#pragma once

#include"BasePlayerAttackBehavior.h"
#include"math/Easing.h"
#include"Collider/AABBCollider.h"

class PlayerKikAttack : public BasePlayerAttackBehavior {
private:
	enum class Step {
		KIK,
		RETUNROOT,
	};
/// ===================================================
///  private varians
/// ===================================================
	Step step_;
	std::unique_ptr<AABBCollider>weakikCollider_;
	float kikTime_;
	bool isCollision_;

public:
	//コンストラクタ
	PlayerKikAttack(Player* boss);
	~PlayerKikAttack();

	void Update()override;
	void Debug()override;


	void OnCollisionEnter(const ColliderInfo& other);
	void OnCollisionStay(const ColliderInfo& other);
	
};
