#pragma once

#include"BaseEnemyBehavior.h"
#include"Easing.h"

class EnemyJump : public BaseEnemyBehaivor {
private:
	enum class Step {
		BOUND,
		RETUNROOT,
	};
private:

	Step step_;
	float jumpSpeed_;


public:
	//コンストラクタ
	EnemyJump(BaseEnemy* boss);
	~EnemyJump();


	void Update()override;
	void Debug()override;

};
