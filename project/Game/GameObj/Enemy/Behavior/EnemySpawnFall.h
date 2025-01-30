#pragma once

#include"BaseEnemyBehavior.h"
#include"Easing.h"
#include "Particle/ParticleEmitter.h"
#include "Audio/Audio.h"

class EnemySpawnFall : public BaseEnemyBehaivor {
private:

	enum class Step {
		FALL,
		BOUND,
	};

private:

	Step step_;
	float speed_;
	float boundSpeed_;
	float rotateX_;
	float rotateSpeed_;

	Easing gotoFrontEase_;

	ParticleEmitter star1_;
	ParticleEmitter star2_;

	SoundData katunSE_;

public:
	//コンストラクタ
	EnemySpawnFall(BaseEnemy* boss);
	~EnemySpawnFall();


	void Update()override;
	void Debug()override;

};
