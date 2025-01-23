#pragma once

#include"BaseEnemyBehavior.h"
#include"Easing.h"

class EnemyExplotion : public BaseEnemyBehaivor {
private:
	enum class Step {
		DEATH,
	};
private:

	Step step_;
	

public:
	//コンストラクタ
	EnemyExplotion(BaseEnemy* boss);
	~EnemyExplotion();


	void Update()override;
	void Debug()override;

};
