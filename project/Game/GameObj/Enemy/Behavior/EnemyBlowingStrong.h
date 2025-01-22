#pragma once

#include"BaseEnemyBehavior.h"
#include"Easing.h"

class EnemyBlowingStrong: public BaseEnemyBehaivor {
private:
	enum class Step {
		BLOW,
		RETUNROOT,
	};
private:

	Step step_;
	float jumpSpeed_;

	float rotateZ_;
	float rotateSpeed_;


public:
	//コンストラクタ
	EnemyBlowingStrong(BaseEnemy* boss);
	~EnemyBlowingStrong();


	void Update()override;
	void Debug()override;

};
