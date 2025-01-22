#pragma once

#include"BaseEnemyBehavior.h"
#include"Easing.h"

class EnemyBound : public BaseEnemyBehaivor {
private:
	enum class Step {
		BOUND,
		REBOUND,
	};
private:

	Step step_;
	float jumpSpeed_;

	float rotateZ_;
	float rotateSpeed_;


public:
	//コンストラクタ
	EnemyBound(BaseEnemy* boss);
	~EnemyBound();


	void Update()override;
	void Debug()override;

};
