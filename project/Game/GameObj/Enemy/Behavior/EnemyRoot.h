#pragma once

#include"BaseEnemyBehavior.h"
#include"Easing.h"

class EnemyRoot : public BaseEnemyBehaivor {
private:
	enum class Step {
		BOUND,
		RETUNROOT,
	};
private:

	Step step_;
	

public:
	//コンストラクタ
	EnemyRoot(BaseEnemy* boss);
	~EnemyRoot();


	void Update()override;
	void Debug()override;

};
