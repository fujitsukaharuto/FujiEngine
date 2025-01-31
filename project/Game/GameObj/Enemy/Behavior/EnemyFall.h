#pragma once

#include"BaseEnemyBehavior.h"
#include"Easing.h"

class EnemyFall : public BaseEnemyBehaivor {
private:
	enum class Step {
		FALL,
		LANDING,
		CHANGEATTACK,
	};
private:

	
	Step step_;
	

	//land
	float waitTime_;

public:
	//コンストラクタ
	EnemyFall(BaseEnemy* boss);
	~EnemyFall();


	void Update()override;
	void Debug()override;

	
	
};
