#pragma once

#include"BaseEnemyBehavior.h"
#include"Easing.h"

class EnemyBlowingWeak : public BaseEnemyBehaivor {
private:
	enum class Step {
		BOUND,
		RETUNROOT,
	};
private:

	Step step_;
	float jumpSpeed_;

	float rotateZ_;
	float rotateSpeed_;


public:
	//コンストラクタ
	EnemyBlowingWeak(BaseEnemy* boss);
	~EnemyBlowingWeak();


	void Update()override;
	void Debug()override;

};
