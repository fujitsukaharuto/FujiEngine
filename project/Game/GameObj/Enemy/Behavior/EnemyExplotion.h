#pragma once

#include"BaseEnemyBehavior.h"
#include "Particle/ParticleEmitter.h"
#include"Easing.h"

class EnemyExplotion : public BaseEnemyBehaivor {
private:
	enum class Step {
		WAIT,
		DEATH,
	};
private:

	Step step_;
	
	/// emitter
	ParticleEmitter bombEmit1_;
	ParticleEmitter bombEmit2_;
	ParticleEmitter bombEmit3_;

	float waitTime_;
	float waitTimeMax_;

public:
	//コンストラクタ
	EnemyExplotion(BaseEnemy* boss);
	~EnemyExplotion();


	void Update()override;
	void Debug()override;

};
