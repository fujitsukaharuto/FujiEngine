#pragma once

#include"BaseEnemyBehavior.h"


class EnemyExplotion : public BaseEnemyBehaivor {
private:
	enum class Step {
		WAIT,
		DEATH,
	};
private:

	Step step_;


	float waitTime_;
	float waitTimeMax_;

public:
	//コンストラクタ
	EnemyExplotion(BaseEnemy* boss);
	~EnemyExplotion();


	void Update()override;
	void Debug()override;

};
