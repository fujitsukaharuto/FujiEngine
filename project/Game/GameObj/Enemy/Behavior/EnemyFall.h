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
	float rotateAmplitude_;      // 揺れ幅R
	float rotateFrequency_;      // 揺れる速さR
	float translateAmplitude_;    // 揺れ幅T
	float translateFrequency_;   // 揺れる速さT
	float zRotation_;            //	Z回転
	float xTranslation_;         // X変位

	Easing speedEase_;
	float   maxSpeed_;
	

	//land
	float waitTime_;

public:
	//コンストラクタ
	EnemyFall(BaseEnemy* boss);
	~EnemyFall();


	void Update()override;
	void Debug()override;

	void SpeedChangeMethod();

};
