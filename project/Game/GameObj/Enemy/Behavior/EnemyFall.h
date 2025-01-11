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
	// fall
	float fallspeed_;
	float time_;
	 float amplitude_;    // 揺れ幅
	 float frequency_;   // 揺れる速さ
	 float zRotation_;

	//land
	float waitTime_;

public:
	//コンストラクタ
	EnemyFall(BaseEnemy* boss);
	~EnemyFall();


	void Update()override;
	void Debug()override;

};
