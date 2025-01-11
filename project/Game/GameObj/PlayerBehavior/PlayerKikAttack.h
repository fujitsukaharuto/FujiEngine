#pragma once

#include"BasePlayerBehavior.h"
#include"math/Easing.h"
#include"Collider/AABBCollider.h"

class PlayerKikAttack : public BasePlayerBehavior {
private:

/// ===================================================
///  private varians
/// ===================================================
	std::unique_ptr<AABBCollider>normalKikCollider_;
	

public:
	//コンストラクタ
	PlayerKikAttack(Player* boss);
	~PlayerKikAttack();

	void Update()override;
	void Debug()override;


	void OnCollisionEnter(const ColliderInfo& other);
	void OnCollisionStay(const ColliderInfo& other);
	
};
