#pragma once

#include"BaseEnemyState.h"
#include"Easing.h"

class EnemyNoneState : public BaseEnemyState {
private:
	enum class Step {
		BOUND,
		RETUNROOT,
	};
private:

	Step step_;
	

public:
	//コンストラクタ
	EnemyNoneState(BaseEnemy* boss);
	~EnemyNoneState();


	void Update()override;
	void Debug()override;

};
