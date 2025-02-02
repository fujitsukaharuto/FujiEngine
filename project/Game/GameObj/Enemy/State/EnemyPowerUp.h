#pragma once

#include"BaseEnemyState.h"
#include"Easing.h"
#include "Audio/Audio.h"

class EnemyPowerUp : public BaseEnemyState {
private:
	enum class Step {
		SCALEUP,
		RETUNROOT,
	};
private:

	Step step_;
	Vector3 saveScale_;
	Vector3 scaleUpParm_;
	Easing ease_;

	float stopTime_;
	float stopTimeMax_;

	SoundData sizeupSE_;

public:
	//コンストラクタ
	EnemyPowerUp(BaseEnemy* boss);
	~EnemyPowerUp();


	void Update()override;
	void Debug()override;

};
