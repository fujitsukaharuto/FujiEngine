#pragma once

#include"BaseEnemyBehavior.h"
#include"Easing.h"

class EnemyBlowingWeak : public BaseEnemyBehaivor {
private:
	enum class Step {
		BLOW,
		RETUNROOT,
	};
private:

	Step step_;
	float blowPower_;
	Vector3 direction_;

	float rotateZ_;
	float rotateSpeed_;


public:
	//コンストラクタ
	EnemyBlowingWeak(BaseEnemy* boss);
	~EnemyBlowingWeak();


	void Update()override;
	void Debug()override;

};
